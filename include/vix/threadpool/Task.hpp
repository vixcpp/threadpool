/**
 *
 * @file Task.hpp
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
#ifndef VIX_THREADPOOL_TASK_HPP
#define VIX_THREADPOOL_TASK_HPP

#include <chrono>
#include <cstdint>
#include <exception>
#include <utility>

#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/detail/MoveOnlyFunction.hpp>

namespace vix::threadpool
{
  /**
   * @brief Callable type stored by a threadpool task.
   *
   * The callable is move-only so tasks can own packaged tasks, promises,
   * unique pointers, and other non-copyable state.
   */
  using TaskFunction = detail::MoveOnlyFunction<void()>;

  /**
   * @brief Unit of work executed by the thread pool.
   *
   * Task stores:
   * - a stable task id
   * - a move-only callable
   * - task options
   * - lifecycle status
   * - execution result
   * - sequence number for stable queue ordering
   * - captured exception, if execution failed
   */
  class Task
  {
  public:
    /**
     * @brief Clock used for task timing.
     */
    using clock = std::chrono::steady_clock;

    /**
     * @brief Construct an invalid empty task.
     */
    Task() noexcept
        : id_(invalid_task_id),
          function_(),
          options_(),
          status_(TaskStatus::created),
          result_(TaskResult::none),
          sequence_(0),
          exception_(nullptr),
          created_at_(clock::now()),
          started_at_(),
          finished_at_()
    {
    }

    /**
     * @brief Construct a task from an id, callable, options, and sequence.
     *
     * @param id Unique task identifier.
     * @param function Task callable.
     * @param options Task options.
     * @param sequence Monotonic sequence used for stable queue ordering.
     */
    Task(
        TaskId id,
        TaskFunction function,
        TaskOptions options = TaskOptions{},
        std::uint64_t sequence = 0) noexcept
        : id_(id),
          function_(std::move(function)),
          options_(std::move(options)),
          status_(TaskStatus::created),
          result_(TaskResult::none),
          sequence_(sequence),
          exception_(nullptr),
          created_at_(clock::now()),
          started_at_(),
          finished_at_()
    {
    }

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    /**
     * @brief Move-construct a task.
     */
    Task(Task &&) noexcept = default;

    /**
     * @brief Move-assign a task.
     *
     * @return Reference to this task.
     */
    Task &operator=(Task &&) noexcept = default;

    /**
     * @brief Destroy the task.
     */
    ~Task() = default;

    /**
     * @brief Return the task id.
     *
     * @return Task id.
     */
    [[nodiscard]] TaskId id() const noexcept
    {
      return id_;
    }

    /**
     * @brief Return the task options.
     *
     * @return Task options.
     */
    [[nodiscard]] const TaskOptions &options() const noexcept
    {
      return options_;
    }

    /**
     * @brief Return mutable task options.
     *
     * @return Mutable task options.
     */
    [[nodiscard]] TaskOptions &options() noexcept
    {
      return options_;
    }

    /**
     * @brief Return the task priority.
     *
     * @return Task priority.
     */
    [[nodiscard]] TaskPriority priority() const noexcept
    {
      return options_.priority;
    }

    /**
     * @brief Return the task sequence number.
     *
     * @return Monotonic sequence number.
     */
    [[nodiscard]] std::uint64_t sequence() const noexcept
    {
      return sequence_;
    }

    /**
     * @brief Return the current task status.
     *
     * @return Task status.
     */
    [[nodiscard]] TaskStatus status() const noexcept
    {
      return status_;
    }

    /**
     * @brief Return the current task result.
     *
     * @return Task result.
     */
    [[nodiscard]] TaskResult result() const noexcept
    {
      return result_;
    }

    /**
     * @brief Return the captured exception.
     *
     * @return Captured exception pointer, or nullptr.
     */
    [[nodiscard]] std::exception_ptr exception() const noexcept
    {
      return exception_;
    }

    /**
     * @brief Return whether the task contains executable work.
     *
     * @return true if the task has a valid id and callable.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return is_valid_task_id(id_) && static_cast<bool>(function_);
    }

    /**
     * @brief Check whether the task may be queued.
     *
     * @return true if the task is valid and not terminal.
     */
    [[nodiscard]] bool schedulable() const noexcept
    {
      return valid() && !is_terminal(status_);
    }

    /**
     * @brief Check whether the task reached a terminal state.
     *
     * @return true if the task is completed, failed, cancelled, timed out, or rejected.
     */
    [[nodiscard]] bool done() const noexcept
    {
      return is_terminal(status_);
    }

    /**
     * @brief Check whether the task is currently running.
     *
     * @return true if task status is running.
     */
    [[nodiscard]] bool running() const noexcept
    {
      return status_ == TaskStatus::running;
    }

    /**
     * @brief Check whether the task is queued.
     *
     * @return true if task status is queued.
     */
    [[nodiscard]] bool queued() const noexcept
    {
      return status_ == TaskStatus::queued;
    }

    /**
     * @brief Check whether the task completed successfully.
     *
     * @return true if task result is success.
     */
    [[nodiscard]] bool succeeded() const noexcept
    {
      return result_ == TaskResult::success;
    }

    /**
     * @brief Check whether the task failed.
     *
     * @return true if task result represents a failure.
     */
    [[nodiscard]] bool failed() const noexcept
    {
      return is_failure(result_);
    }

    /**
     * @brief Return the task creation time.
     *
     * @return Creation time point.
     */
    [[nodiscard]] clock::time_point created_at() const noexcept
    {
      return created_at_;
    }

    /**
     * @brief Return the task start time.
     *
     * The value is meaningful only after the task starts running.
     *
     * @return Start time point.
     */
    [[nodiscard]] clock::time_point started_at() const noexcept
    {
      return started_at_;
    }

    /**
     * @brief Return the task finish time.
     *
     * The value is meaningful only after the task reaches a terminal state.
     *
     * @return Finish time point.
     */
    [[nodiscard]] clock::time_point finished_at() const noexcept
    {
      return finished_at_;
    }

    /**
     * @brief Return the observed execution duration.
     *
     * If the task has not finished, the duration is measured from start to now.
     * If the task has not started, zero is returned.
     *
     * @return Execution duration.
     */
    [[nodiscard]] clock::duration execution_duration() const noexcept
    {
      if (started_at_ == clock::time_point{})
      {
        return clock::duration::zero();
      }

      if (finished_at_ == clock::time_point{})
      {
        return clock::now() - started_at_;
      }

      return finished_at_ - started_at_;
    }

    /**
     * @brief Mark the task as queued.
     */
    void mark_queued() noexcept
    {
      if (!done())
      {
        status_ = TaskStatus::queued;
      }
    }

    /**
     * @brief Mark the task as rejected.
     */
    void mark_rejected() noexcept
    {
      status_ = TaskStatus::rejected;
      result_ = TaskResult::rejected;
      finished_at_ = clock::now();
    }

    /**
     * @brief Mark the task as cancelled.
     */
    void mark_cancelled() noexcept
    {
      status_ = TaskStatus::cancelled;
      result_ = TaskResult::cancelled;
      finished_at_ = clock::now();
    }

    /**
     * @brief Mark the task as timed out.
     */
    void mark_timed_out() noexcept
    {
      status_ = TaskStatus::timed_out;
      result_ = TaskResult::timeout;
      finished_at_ = clock::now();
    }

    /**
     * @brief Execute the task callable.
     *
     * This function catches all exceptions so they never escape worker threads.
     *
     * @return Final task result.
     */
    TaskResult run() noexcept
    {
      if (!valid())
      {
        status_ = TaskStatus::failed;
        result_ = TaskResult::failure;
        finished_at_ = clock::now();
        return result_;
      }

      if (options_.cancellation.cancelled())
      {
        mark_cancelled();
        return result_;
      }

      if (options_.deadline.expired())
      {
        mark_timed_out();
        return result_;
      }

      status_ = TaskStatus::running;
      started_at_ = clock::now();

      try
      {
        function_();

        finished_at_ = clock::now();

        if (options_.timeout.expired(finished_at_ - started_at_) ||
            options_.deadline.expired_at(finished_at_))
        {
          status_ = TaskStatus::timed_out;
          result_ = TaskResult::timeout;
          return result_;
        }

        if (options_.cancellation.cancelled())
        {
          status_ = TaskStatus::cancelled;
          result_ = TaskResult::cancelled;
          return result_;
        }

        status_ = TaskStatus::completed;
        result_ = TaskResult::success;
        return result_;
      }
      catch (...)
      {
        exception_ = std::current_exception();
        finished_at_ = clock::now();
        status_ = TaskStatus::failed;
        result_ = TaskResult::failure;
        return result_;
      }
    }

  private:
    /**
     * @brief Unique task identifier.
     */
    TaskId id_;

    /**
     * @brief Callable executed by the task.
     */
    TaskFunction function_;

    /**
     * @brief Task execution options.
     */
    TaskOptions options_;

    /**
     * @brief Current task lifecycle status.
     */
    TaskStatus status_;

    /**
     * @brief Current task result.
     */
    TaskResult result_;

    /**
     * @brief Sequence number used for stable queue ordering.
     */
    std::uint64_t sequence_;

    /**
     * @brief Exception captured during task execution.
     */
    std::exception_ptr exception_;

    /**
     * @brief Time at which the task object was created.
     */
    clock::time_point created_at_;

    /**
     * @brief Time at which task execution started.
     */
    clock::time_point started_at_;

    /**
     * @brief Time at which task execution finished.
     */
    clock::time_point finished_at_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_HPP
