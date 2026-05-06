/**
 *
 * @file TaskHandle.hpp
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
#ifndef VIX_THREADPOOL_TASK_HANDLE_HPP
#define VIX_THREADPOOL_TASK_HANDLE_HPP

#include <utility>

#include <vix/threadpool/CancellationSource.hpp>
#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/ThreadPoolError.hpp>

namespace vix::threadpool
{
  /**
   * @brief User-facing handle for a submitted threadpool task.
   *
   * TaskHandle combines:
   * - the task identifier
   * - the future used to wait for the result
   * - the cancellation source used to request cooperative cancellation
   *
   * The handle is move-only because the contained Future is move-only.
   *
   * @tparam T Result type produced by the task.
   */
  template <class T>
  class TaskHandle
  {
  public:
    /**
     * @brief Result value type.
     */
    using value_type = T;

    /**
     * @brief Construct an invalid task handle.
     */
    TaskHandle() noexcept
        : id_(invalid_task_id),
          future_(),
          cancellation_()
    {
    }

    /**
     * @brief Construct a task handle.
     *
     * @param id Task identifier.
     * @param future Future associated with the task result.
     * @param cancellation Cancellation source linked to the task.
     */
    TaskHandle(
        TaskId id,
        Future<T> future,
        CancellationSource cancellation = CancellationSource{})
        : id_(id),
          future_(std::move(future)),
          cancellation_(std::move(cancellation))
    {
    }

    TaskHandle(const TaskHandle &) = delete;
    TaskHandle &operator=(const TaskHandle &) = delete;

    /**
     * @brief Move-construct a task handle.
     */
    TaskHandle(TaskHandle &&) noexcept = default;

    /**
     * @brief Move-assign a task handle.
     *
     * @return Reference to this handle.
     */
    TaskHandle &operator=(TaskHandle &&) noexcept = default;

    /**
     * @brief Destroy the task handle.
     */
    ~TaskHandle() = default;

    /**
     * @brief Return the task identifier.
     *
     * @return Task id.
     */
    [[nodiscard]] TaskId id() const noexcept
    {
      return id_;
    }

    /**
     * @brief Check whether this handle refers to a valid task.
     *
     * @return true if the task id and future are valid.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return is_valid_task_id(id_) && future_.valid();
    }

    /**
     * @brief Bool conversion.
     *
     * @return true if this handle is valid.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return valid();
    }

    /**
     * @brief Request cooperative cancellation for the task.
     *
     * This does not forcibly stop a running C++ function. The task must observe
     * its cancellation token or be skipped before execution.
     */
    void cancel() noexcept
    {
      cancellation_.request_cancel();
    }

    /**
     * @brief Check whether cancellation has been requested.
     *
     * @return true if cancellation was requested.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return cancellation_.cancelled();
    }

    /**
     * @brief Check whether the task result is ready.
     *
     * @return true if the future is valid and ready.
     */
    [[nodiscard]] bool ready() const
    {
      return future_.ready();
    }

    /**
     * @brief Wait until the task result is ready.
     */
    void wait() const
    {
      future_.wait();
    }

    /**
     * @brief Wait and retrieve the task result.
     *
     * This consumes the result. Calling get() more than once throws
     * std::future_error.
     *
     * @return Task result value.
     */
    T get()
    {
      return future_.get();
    }

    /**
     * @brief Return the current task status stored by the future state.
     *
     * @return Task status.
     */
    [[nodiscard]] TaskStatus status() const
    {
      return future_.status();
    }

    /**
     * @brief Return the current task result stored by the future state.
     *
     * @return Task result.
     */
    [[nodiscard]] TaskResult result() const
    {
      return future_.result();
    }

    /**
     * @brief Return the current threadpool error stored by the future state.
     *
     * @return Threadpool error code.
     */
    [[nodiscard]] ThreadPoolErrc error() const
    {
      return future_.error();
    }

    /**
     * @brief Return mutable access to the underlying future.
     *
     * @return Future reference.
     */
    [[nodiscard]] Future<T> &future() noexcept
    {
      return future_;
    }

    /**
     * @brief Return const access to the underlying future.
     *
     * @return Future reference.
     */
    [[nodiscard]] const Future<T> &future() const noexcept
    {
      return future_;
    }

    /**
     * @brief Return the cancellation source linked to this handle.
     *
     * @return Cancellation source reference.
     */
    [[nodiscard]] CancellationSource &cancellation_source() noexcept
    {
      return cancellation_;
    }

    /**
     * @brief Return the cancellation source linked to this handle.
     *
     * @return Cancellation source reference.
     */
    [[nodiscard]] const CancellationSource &cancellation_source() const noexcept
    {
      return cancellation_;
    }

  private:
    /**
     * @brief Task identifier.
     */
    TaskId id_;

    /**
     * @brief Future associated with the task result.
     */
    Future<T> future_;

    /**
     * @brief Cancellation source linked to the task.
     */
    CancellationSource cancellation_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_HANDLE_HPP
