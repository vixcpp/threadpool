/**
 *
 * @file SharedState.hpp
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
#ifndef VIX_THREADPOOL_SHARED_STATE_HPP
#define VIX_THREADPOOL_SHARED_STATE_HPP

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <future>

#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/ThreadPoolError.hpp>

namespace vix::threadpool
{
  /**
   * @brief Shared asynchronous state used by Future and Promise.
   *
   * SharedState stores either:
   * - a value of type T
   * - an exception
   * - a threadpool error code
   *
   * It is protected by a mutex and condition variable so one or more waiters can
   * wait until the state becomes ready.
   *
   * @tparam T Stored result type.
   */
  template <class T>
  class SharedState
  {
    static_assert(!std::is_reference_v<T>,
                  "SharedState<T> does not support reference types");

  public:
    /**
     * @brief Stored value type.
     */
    using value_type = T;

    /**
     * @brief Construct an empty non-ready shared state.
     */
    SharedState() noexcept
        : value_(),
          exception_(nullptr),
          error_(ThreadPoolErrc::ok),
          status_(TaskStatus::created),
          result_(TaskResult::none),
          ready_(false),
          retrieved_(false)
    {
    }

    SharedState(const SharedState &) = delete;
    SharedState &operator=(const SharedState &) = delete;

    /**
     * @brief Store a value and mark the state ready.
     *
     * Calls after the state is already ready are ignored.
     *
     * @param value Value to store.
     */
    void set_value(T value)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        value_.emplace(std::move(value));
        error_ = ThreadPoolErrc::ok;
        status_ = TaskStatus::completed;
        result_ = TaskResult::success;
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Construct the value in-place and mark the state ready.
     *
     * Calls after the state is already ready are ignored.
     *
     * @tparam Args Constructor argument types.
     * @param args Constructor arguments forwarded to T.
     */
    template <class... Args>
    void emplace_value(Args &&...args)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        value_.emplace(std::forward<Args>(args)...);
        error_ = ThreadPoolErrc::ok;
        status_ = TaskStatus::completed;
        result_ = TaskResult::success;
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Store an exception and mark the state ready.
     *
     * Calls after the state is already ready are ignored.
     *
     * @param exception Exception to store.
     */
    void set_exception(std::exception_ptr exception)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        exception_ = exception;
        error_ = ThreadPoolErrc::internal_error;
        status_ = TaskStatus::failed;
        result_ = TaskResult::failure;
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Store an error code and mark the state ready.
     *
     * Calls after the state is already ready are ignored.
     *
     * @param error Threadpool error code.
     */
    void set_error(ThreadPoolErrc error)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        error_ = error;
        status_ = status_from_error(error);
        result_ = result_from_error(error);
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Wait until the state is ready.
     */
    void wait() const
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]()
               { return ready_; });
    }

    /**
     * @brief Check whether the state is ready.
     *
     * @return true if a value, exception, or error has been stored.
     */
    [[nodiscard]] bool ready() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return ready_;
    }

    /**
     * @brief Check whether the result has already been retrieved.
     *
     * @return true if get() has consumed the state, false otherwise.
     */
    [[nodiscard]] bool retrieved() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return retrieved_;
    }

    /**
     * @brief Return the stored task status.
     *
     * @return Stored task status.
     */
    [[nodiscard]] TaskStatus status() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return status_;
    }

    /**
     * @brief Return the stored task result.
     *
     * @return Stored task result.
     */
    [[nodiscard]] TaskResult result() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return result_;
    }

    /**
     * @brief Return the stored error code.
     *
     * @return Stored threadpool error code.
     */
    [[nodiscard]] ThreadPoolErrc error() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return error_;
    }

    /**
     * @brief Wait and consume the stored value.
     *
     * If an exception was stored, it is rethrown. If an error was stored, a
     * std::system_error is thrown.
     *
     * @return Stored value.
     */
    T get()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]()
               { return ready_; });

      if (retrieved_)
      {
        throw std::future_error(std::future_errc::future_already_retrieved);
      }

      retrieved_ = true;

      if (exception_)
      {
        std::rethrow_exception(exception_);
      }

      if (error_ != ThreadPoolErrc::ok)
      {
        throw std::system_error(make_error_code(error_));
      }

      return std::move(*value_);
    }

  private:
    /**
     * @brief Convert a threadpool error code to a task status.
     *
     * @param error Threadpool error code.
     * @return Matching task status.
     */
    [[nodiscard]] static constexpr TaskStatus status_from_error(ThreadPoolErrc error) noexcept
    {
      switch (error)
      {
      case ThreadPoolErrc::ok:
        return TaskStatus::completed;
      case ThreadPoolErrc::cancelled:
        return TaskStatus::cancelled;
      case ThreadPoolErrc::timeout:
        return TaskStatus::timed_out;
      case ThreadPoolErrc::rejected:
      case ThreadPoolErrc::queue_full:
      case ThreadPoolErrc::stopped:
        return TaskStatus::rejected;
      case ThreadPoolErrc::invalid_argument:
      case ThreadPoolErrc::not_ready:
      case ThreadPoolErrc::not_supported:
      case ThreadPoolErrc::internal_error:
      default:
        return TaskStatus::failed;
      }
    }

    /**
     * @brief Convert a threadpool error code to a task result.
     *
     * @param error Threadpool error code.
     * @return Matching task result.
     */
    [[nodiscard]] static constexpr TaskResult result_from_error(ThreadPoolErrc error) noexcept
    {
      switch (error)
      {
      case ThreadPoolErrc::ok:
        return TaskResult::success;
      case ThreadPoolErrc::cancelled:
        return TaskResult::cancelled;
      case ThreadPoolErrc::timeout:
        return TaskResult::timeout;
      case ThreadPoolErrc::rejected:
      case ThreadPoolErrc::queue_full:
      case ThreadPoolErrc::stopped:
        return TaskResult::rejected;
      case ThreadPoolErrc::invalid_argument:
      case ThreadPoolErrc::not_ready:
      case ThreadPoolErrc::not_supported:
      case ThreadPoolErrc::internal_error:
      default:
        return TaskResult::failure;
      }
    }

  private:
    /**
     * @brief Stored value.
     */
    std::optional<T> value_;

    /**
     * @brief Stored exception.
     */
    std::exception_ptr exception_;

    /**
     * @brief Stored threadpool error code.
     */
    ThreadPoolErrc error_;

    /**
     * @brief Stored task status.
     */
    TaskStatus status_;

    /**
     * @brief Stored task result.
     */
    TaskResult result_;

    /**
     * @brief Whether the state is ready.
     */
    bool ready_;

    /**
     * @brief Whether the result has been retrieved.
     */
    bool retrieved_;

    /**
     * @brief Mutex protecting the shared state.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable used to wait for readiness.
     */
    mutable std::condition_variable cv_;
  };

  /**
   * @brief Shared asynchronous state specialization for void results.
   */
  template <>
  class SharedState<void>
  {
  public:
    /**
     * @brief Stored value type.
     */
    using value_type = void;

    /**
     * @brief Construct an empty non-ready shared state.
     */
    SharedState() noexcept
        : exception_(nullptr),
          error_(ThreadPoolErrc::ok),
          status_(TaskStatus::created),
          result_(TaskResult::none),
          ready_(false),
          retrieved_(false)
    {
    }

    SharedState(const SharedState &) = delete;
    SharedState &operator=(const SharedState &) = delete;

    /**
     * @brief Mark the void state as successfully completed.
     *
     * Calls after the state is already ready are ignored.
     */
    void set_value()
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        error_ = ThreadPoolErrc::ok;
        status_ = TaskStatus::completed;
        result_ = TaskResult::success;
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Store an exception and mark the state ready.
     *
     * Calls after the state is already ready are ignored.
     *
     * @param exception Exception to store.
     */
    void set_exception(std::exception_ptr exception)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        exception_ = exception;
        error_ = ThreadPoolErrc::internal_error;
        status_ = TaskStatus::failed;
        result_ = TaskResult::failure;
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Store an error code and mark the state ready.
     *
     * Calls after the state is already ready are ignored.
     *
     * @param error Threadpool error code.
     */
    void set_error(ThreadPoolErrc error)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (ready_)
        {
          return;
        }

        error_ = error;
        status_ = status_from_error(error);
        result_ = result_from_error(error);
        ready_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Wait until the state is ready.
     */
    void wait() const
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]()
               { return ready_; });
    }

    /**
     * @brief Check whether the state is ready.
     *
     * @return true if completion, exception, or error has been stored.
     */
    [[nodiscard]] bool ready() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return ready_;
    }

    /**
     * @brief Check whether the result has already been retrieved.
     *
     * @return true if get() has consumed the state, false otherwise.
     */
    [[nodiscard]] bool retrieved() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return retrieved_;
    }

    /**
     * @brief Return the stored task status.
     *
     * @return Stored task status.
     */
    [[nodiscard]] TaskStatus status() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return status_;
    }

    /**
     * @brief Return the stored task result.
     *
     * @return Stored task result.
     */
    [[nodiscard]] TaskResult result() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return result_;
    }

    /**
     * @brief Return the stored error code.
     *
     * @return Stored threadpool error code.
     */
    [[nodiscard]] ThreadPoolErrc error() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return error_;
    }

    /**
     * @brief Wait and consume the stored completion.
     *
     * If an exception was stored, it is rethrown. If an error was stored, a
     * std::system_error is thrown.
     */
    void get()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]()
               { return ready_; });

      if (retrieved_)
      {
        throw std::future_error(std::future_errc::future_already_retrieved);
      }

      retrieved_ = true;

      if (exception_)
      {
        std::rethrow_exception(exception_);
      }

      if (error_ != ThreadPoolErrc::ok)
      {
        throw std::system_error(make_error_code(error_));
      }
    }

  private:
    /**
     * @brief Convert a threadpool error code to a task status.
     *
     * @param error Threadpool error code.
     * @return Matching task status.
     */
    [[nodiscard]] static constexpr TaskStatus status_from_error(ThreadPoolErrc error) noexcept
    {
      switch (error)
      {
      case ThreadPoolErrc::ok:
        return TaskStatus::completed;
      case ThreadPoolErrc::cancelled:
        return TaskStatus::cancelled;
      case ThreadPoolErrc::timeout:
        return TaskStatus::timed_out;
      case ThreadPoolErrc::rejected:
      case ThreadPoolErrc::queue_full:
      case ThreadPoolErrc::stopped:
        return TaskStatus::rejected;
      case ThreadPoolErrc::invalid_argument:
      case ThreadPoolErrc::not_ready:
      case ThreadPoolErrc::not_supported:
      case ThreadPoolErrc::internal_error:
      default:
        return TaskStatus::failed;
      }
    }

    /**
     * @brief Convert a threadpool error code to a task result.
     *
     * @param error Threadpool error code.
     * @return Matching task result.
     */
    [[nodiscard]] static constexpr TaskResult result_from_error(ThreadPoolErrc error) noexcept
    {
      switch (error)
      {
      case ThreadPoolErrc::ok:
        return TaskResult::success;
      case ThreadPoolErrc::cancelled:
        return TaskResult::cancelled;
      case ThreadPoolErrc::timeout:
        return TaskResult::timeout;
      case ThreadPoolErrc::rejected:
      case ThreadPoolErrc::queue_full:
      case ThreadPoolErrc::stopped:
        return TaskResult::rejected;
      case ThreadPoolErrc::invalid_argument:
      case ThreadPoolErrc::not_ready:
      case ThreadPoolErrc::not_supported:
      case ThreadPoolErrc::internal_error:
      default:
        return TaskResult::failure;
      }
    }

  private:
    /**
     * @brief Stored exception.
     */
    std::exception_ptr exception_;

    /**
     * @brief Stored threadpool error code.
     */
    ThreadPoolErrc error_;

    /**
     * @brief Stored task status.
     */
    TaskStatus status_;

    /**
     * @brief Stored task result.
     */
    TaskResult result_;

    /**
     * @brief Whether the state is ready.
     */
    bool ready_;

    /**
     * @brief Whether the completion has been retrieved.
     */
    bool retrieved_;

    /**
     * @brief Mutex protecting the shared state.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable used to wait for readiness.
     */
    mutable std::condition_variable cv_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_SHARED_STATE_HPP
