/**
 *
 * @file Scope.hpp
 * @author Gaspard Kirira
 *
 * Copyright 2025, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by a MIT license
 * that can be found in the License file.
 *
 * Vix.cpp
 *
 */
#ifndef VIX_THREADPOOL_SCOPE_HPP
#define VIX_THREADPOOL_SCOPE_HPP

#include <cstddef>
#include <exception>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include <vix/threadpool/CancellationSource.hpp>
#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>

namespace vix::threadpool
{
  /**
   * @brief Structured concurrency helper for scoped threadpool tasks.
   *
   * Scope groups tasks submitted to a ThreadPool and guarantees that they are
   * waited for before the scope object is destroyed.
   *
   * This is useful when a function starts several concurrent operations and must
   * ensure they are completed before returning.
   *
   * Behavior:
   * - spawn() submits work to the pool
   * - wait() waits for all spawned tasks
   * - cancel() requests cooperative cancellation for future spawned tasks
   * - destructor calls wait() and swallows exceptions
   *
   * Exceptions from tasks are captured and rethrown by wait_and_rethrow().
   */
  class Scope
  {
  public:
    /**
     * @brief Construct a scope bound to a thread pool.
     *
     * @param pool Thread pool used for spawned tasks.
     */
    explicit Scope(ThreadPool &pool) noexcept
        : pool_(&pool),
          cancellation_(),
          futures_(),
          mutex_(),
          closed_(false)
    {
    }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

    Scope(Scope &&) = delete;
    Scope &operator=(Scope &&) = delete;

    /**
     * @brief Wait for all spawned tasks before destruction.
     *
     * Exceptions are intentionally swallowed in the destructor. Use
     * wait_and_rethrow() when exceptions must be observed.
     */
    ~Scope() noexcept
    {
      try
      {
        wait();
      }
      catch (...)
      {
      }
    }

    /**
     * @brief Spawn one task inside this scope.
     *
     * The task is submitted to the bound ThreadPool and tracked by the scope.
     * A scope-level cancellation token is attached to the task options.
     *
     * @tparam Fn Callable type.
     * @param fn Callable to execute.
     * @param options Task submission options.
     * @return true if the task was accepted for tracking, false otherwise.
     */
    template <class Fn>
    [[nodiscard]] bool spawn(
        Fn &&fn,
        TaskOptions options = TaskOptions{})
    {
      static_assert(
          std::is_invocable_v<std::decay_t<Fn> &>,
          "Scope::spawn requires a callable invocable with no arguments");

      std::lock_guard<std::mutex> lock(mutex_);

      if (closed_ || pool_ == nullptr)
      {
        return false;
      }

      options.set_cancellation(cancellation_.token());

      Future<void> future =
          pool_->submit(
              [function = std::decay_t<Fn>(std::forward<Fn>(fn))]() mutable
              {
                function();
              },
              std::move(options));

      futures_.push_back(std::move(future));
      return true;
    }

    /**
     * @brief Close the scope against new tasks.
     *
     * Existing tasks remain tracked and can still be waited for.
     */
    void close()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
    }

    /**
     * @brief Request cooperative cancellation for all linked tasks.
     *
     * This does not forcibly stop running C++ code. Tasks must observe their
     * cancellation token, or cancellation is observed before execution starts.
     */
    void cancel() noexcept
    {
      cancellation_.request_cancel();
    }

    /**
     * @brief Check whether cancellation has been requested.
     *
     * @return true if scope cancellation was requested.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return cancellation_.cancelled();
    }

    /**
     * @brief Return the cancellation token shared by scoped tasks.
     *
     * @return Cancellation token.
     */
    [[nodiscard]] CancellationToken cancellation_token() const noexcept
    {
      return cancellation_.token();
    }

    /**
     * @brief Wait for all tracked tasks.
     *
     * Exceptions produced by tasks are swallowed. Use wait_and_rethrow() to
     * observe the first task exception after all tasks finish.
     */
    void wait()
    {
      std::vector<Future<void>> futures = take_futures();

      for (auto &future : futures)
      {
        try
        {
          future.get();
        }
        catch (...)
        {
        }
      }
    }

    /**
     * @brief Wait for all tracked tasks and rethrow the first exception.
     *
     * All tasks are waited for before the exception is rethrown.
     */
    void wait_and_rethrow()
    {
      std::vector<Future<void>> futures = take_futures();
      std::exception_ptr firstException = nullptr;

      for (auto &future : futures)
      {
        try
        {
          future.get();
        }
        catch (...)
        {
          if (!firstException)
          {
            firstException = std::current_exception();
          }
        }
      }

      if (firstException)
      {
        std::rethrow_exception(firstException);
      }
    }

    /**
     * @brief Check whether no tracked task is stored.
     *
     * @return true if the scope currently tracks no future.
     */
    [[nodiscard]] bool empty() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return futures_.empty();
    }

    /**
     * @brief Return the number of tracked task futures.
     *
     * @return Tracked future count.
     */
    [[nodiscard]] std::size_t size() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return futures_.size();
    }

    /**
     * @brief Check whether the scope is closed to new tasks.
     *
     * @return true if closed.
     */
    [[nodiscard]] bool closed() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return closed_;
    }

  private:
    /**
     * @brief Move tracked futures out of the scope.
     *
     * The scope is closed before the futures are moved out so no new tasks are
     * accepted while waiting begins.
     *
     * @return Previously tracked futures.
     */
    [[nodiscard]] std::vector<Future<void>> take_futures()
    {
      std::lock_guard<std::mutex> lock(mutex_);

      closed_ = true;

      std::vector<Future<void>> futures;
      futures.swap(futures_);

      return futures;
    }

  private:
    /**
     * @brief Thread pool used for spawning tasks.
     */
    ThreadPool *pool_;

    /**
     * @brief Scope-level cancellation source.
     */
    CancellationSource cancellation_;

    /**
     * @brief Futures tracked by the scope.
     */
    std::vector<Future<void>> futures_;

    /**
     * @brief Mutex protecting tracked futures and closed state.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Whether this scope accepts new tasks.
     */
    bool closed_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_SCOPE_HPP
