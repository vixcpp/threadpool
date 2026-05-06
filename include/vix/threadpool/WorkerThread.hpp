/**
 *
 * @file WorkerThread.hpp
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
#ifndef VIX_THREADPOOL_WORKER_THREAD_HPP
#define VIX_THREADPOOL_WORKER_THREAD_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <utility>

#include <vix/threadpool/WorkerId.hpp>
#include <vix/threadpool/WorkerState.hpp>
#include <vix/threadpool/detail/ThreadName.hpp>
#include <vix/threadpool/this_worker.hpp>

namespace vix::threadpool
{
  /**
   * @brief Small RAII wrapper around one worker std::thread.
   *
   * WorkerThread owns the physical OS thread used by a threadpool worker.
   * It is responsible for:
   * - launching the worker loop
   * - setting thread-local worker context
   * - naming the thread when supported
   * - tracking worker lifecycle state
   * - stopping and joining safely
   *
   * WorkerThread does not own the task queue. It only owns the thread that runs
   * the loop provided by higher-level worker code.
   */
  class WorkerThread
  {
  public:
    /**
     * @brief Function executed by the worker thread.
     */
    using RunFunction = std::function<void()>;

    /**
     * @brief Construct an empty worker thread wrapper.
     */
    WorkerThread() noexcept
        : id_(invalid_worker_id),
          index_(0),
          name_(),
          state_(WorkerState::created),
          stop_requested_(false),
          thread_()
    {
    }

    /**
     * @brief Construct a worker thread wrapper.
     *
     * @param id Worker identifier.
     * @param index Worker index inside the owning pool.
     * @param name Worker thread name.
     */
    WorkerThread(
        WorkerId id,
        std::size_t index,
        std::string name = {})
        : id_(id),
          index_(index),
          name_(std::move(name)),
          state_(WorkerState::created),
          stop_requested_(false),
          thread_()
    {
    }

    WorkerThread(const WorkerThread &) = delete;
    WorkerThread &operator=(const WorkerThread &) = delete;

    /**
     * @brief Move-construct a worker thread wrapper.
     *
     * @param other Source worker thread.
     */
    WorkerThread(WorkerThread &&other) noexcept
        : id_(other.id_),
          index_(other.index_),
          name_(std::move(other.name_)),
          state_(other.state_.load(std::memory_order_acquire)),
          stop_requested_(other.stop_requested_.load(std::memory_order_acquire)),
          thread_(std::move(other.thread_))
    {
      other.id_ = invalid_worker_id;
      other.index_ = 0;
      other.state_.store(WorkerState::stopped, std::memory_order_release);
      other.stop_requested_.store(true, std::memory_order_release);
    }

    /**
     * @brief Move-assign a worker thread wrapper.
     *
     * If this wrapper owns a running thread, it is stopped and joined first.
     *
     * @param other Source worker thread.
     * @return Reference to this wrapper.
     */
    WorkerThread &operator=(WorkerThread &&other) noexcept
    {
      if (this == &other)
      {
        return *this;
      }

      stop();
      join();

      id_ = other.id_;
      index_ = other.index_;
      name_ = std::move(other.name_);
      state_.store(other.state_.load(std::memory_order_acquire),
                   std::memory_order_release);
      stop_requested_.store(
          other.stop_requested_.load(std::memory_order_acquire),
          std::memory_order_release);
      thread_ = std::move(other.thread_);

      other.id_ = invalid_worker_id;
      other.index_ = 0;
      other.state_.store(WorkerState::stopped, std::memory_order_release);
      other.stop_requested_.store(true, std::memory_order_release);

      return *this;
    }

    /**
     * @brief Stop and join the worker thread on destruction.
     */
    ~WorkerThread() noexcept
    {
      stop();
      join();
    }

    /**
     * @brief Start the worker thread.
     *
     * Calling start() on an already joinable worker has no effect.
     *
     * @param fn Worker loop function.
     * @return true if a new thread was started, false otherwise.
     */
    [[nodiscard]] bool start(RunFunction fn)
    {
      if (!fn || thread_.joinable())
      {
        return false;
      }

      stop_requested_.store(false, std::memory_order_release);
      state_.store(WorkerState::idle, std::memory_order_release);

      try
      {
        thread_ = std::thread(
            [this, fn = std::move(fn)]() mutable
            {
              this_worker::set(id_, index_);

              if (!name_.empty())
              {
                detail::set_current_thread_name(name_);
              }

              try
              {
                fn();

                const WorkerState current =
                    state_.load(std::memory_order_acquire);

                if (current != WorkerState::failed)
                {
                  state_.store(WorkerState::stopped, std::memory_order_release);
                }
              }
              catch (...)
              {
                state_.store(WorkerState::failed, std::memory_order_release);
              }

              this_worker::clear();
            });

        if (!name_.empty())
        {
          detail::set_thread_name(thread_, name_);
        }

        return true;
      }
      catch (...)
      {
        state_.store(WorkerState::failed, std::memory_order_release);
        stop_requested_.store(true, std::memory_order_release);
        return false;
      }
    }

    /**
     * @brief Request this worker thread to stop.
     *
     * The running function must observe stop_requested() through the owning
     * Worker or external state. This function does not forcibly kill the thread.
     */
    void stop() noexcept
    {
      stop_requested_.store(true, std::memory_order_release);

      const WorkerState current = state_.load(std::memory_order_acquire);
      if (!is_terminal(current))
      {
        state_.store(WorkerState::stopping, std::memory_order_release);
      }
    }

    /**
     * @brief Join the worker thread if joinable.
     *
     * If join() is called from the same worker thread, the thread is detached to
     * avoid std::terminate or deadlock.
     */
    void join() noexcept
    {
      if (!thread_.joinable())
      {
        return;
      }

      if (thread_.get_id() == std::this_thread::get_id())
      {
        thread_.detach();
        state_.store(WorkerState::stopped, std::memory_order_release);
        return;
      }

      try
      {
        thread_.join();
      }
      catch (...)
      {
        try
        {
          thread_.detach();
        }
        catch (...)
        {
        }
      }

      const WorkerState current = state_.load(std::memory_order_acquire);
      if (current != WorkerState::failed)
      {
        state_.store(WorkerState::stopped, std::memory_order_release);
      }
    }

    /**
     * @brief Check whether the underlying std::thread is joinable.
     *
     * @return true if joinable.
     */
    [[nodiscard]] bool joinable() const noexcept
    {
      return thread_.joinable();
    }

    /**
     * @brief Check whether stop was requested.
     *
     * @return true if the worker should stop.
     */
    [[nodiscard]] bool stop_requested() const noexcept
    {
      return stop_requested_.load(std::memory_order_acquire);
    }

    /**
     * @brief Return the worker id.
     *
     * @return Worker identifier.
     */
    [[nodiscard]] WorkerId id() const noexcept
    {
      return id_;
    }

    /**
     * @brief Return the worker index.
     *
     * @return Worker index inside the owning pool.
     */
    [[nodiscard]] std::size_t index() const noexcept
    {
      return index_;
    }

    /**
     * @brief Return the worker name.
     *
     * @return Worker thread name.
     */
    [[nodiscard]] const std::string &name() const noexcept
    {
      return name_;
    }

    /**
     * @brief Set the worker thread name.
     *
     * If the thread is already running, this attempts to update the platform
     * thread name immediately when supported.
     *
     * @param value New worker name.
     */
    void set_name(std::string value)
    {
      name_ = std::move(value);

      if (thread_.joinable() && !name_.empty())
      {
        detail::set_thread_name(thread_, name_);
      }
    }

    /**
     * @brief Return the current worker lifecycle state.
     *
     * @return Worker state.
     */
    [[nodiscard]] WorkerState state() const noexcept
    {
      return state_.load(std::memory_order_acquire);
    }

    /**
     * @brief Set the current worker lifecycle state.
     *
     * Intended for Worker internals.
     *
     * @param value New worker state.
     */
    void set_state(WorkerState value) noexcept
    {
      state_.store(value, std::memory_order_release);
    }

    /**
     * @brief Return the underlying std::thread id.
     *
     * @return std::thread::id of the worker thread.
     */
    [[nodiscard]] std::thread::id thread_id() const noexcept
    {
      return thread_.get_id();
    }

  private:
    /**
     * @brief Worker identifier.
     */
    WorkerId id_;

    /**
     * @brief Worker index inside the owning pool.
     */
    std::size_t index_;

    /**
     * @brief Requested worker thread name.
     */
    std::string name_;

    /**
     * @brief Current lifecycle state.
     */
    std::atomic<WorkerState> state_;

    /**
     * @brief Whether shutdown was requested.
     */
    std::atomic<bool> stop_requested_;

    /**
     * @brief Owned OS thread.
     */
    std::thread thread_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_WORKER_THREAD_HPP
