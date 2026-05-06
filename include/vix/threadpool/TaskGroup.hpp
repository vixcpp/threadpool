/**
 *
 * @file TaskGroup.hpp
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
#ifndef VIX_THREADPOOL_TASK_GROUP_HPP
#define VIX_THREADPOOL_TASK_GROUP_HPP

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <vector>

#include <vix/threadpool/CancellationSource.hpp>
#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>

namespace vix::threadpool
{
  /**
   * @brief Shared state used to coordinate a group of related tasks.
   *
   * TaskGroupState tracks:
   * - how many tasks are registered
   * - how many tasks are still pending
   * - how many tasks completed, failed, timed out, or were cancelled
   * - the first exception reported by any task
   * - a shared cancellation source for the whole group
   *
   * This class is thread-safe.
   */
  class TaskGroupState
  {
  public:
    /**
     * @brief Construct an empty task group state.
     */
    TaskGroupState()
        : task_ids_(),
          cancellation_(),
          total_tasks_(0),
          pending_tasks_(0),
          completed_tasks_(0),
          failed_tasks_(0),
          cancelled_tasks_(0),
          timed_out_tasks_(0),
          rejected_tasks_(0),
          first_exception_(nullptr),
          closed_(false)
    {
    }

    TaskGroupState(const TaskGroupState &) = delete;
    TaskGroupState &operator=(const TaskGroupState &) = delete;

    /**
     * @brief Register one task in the group.
     *
     * Once a group is closed, new tasks are rejected.
     *
     * @param id Task identifier.
     * @return true if the task was registered, false otherwise.
     */
    [[nodiscard]] bool add_task(TaskId id)
    {
      if (!is_valid_task_id(id))
      {
        return false;
      }

      std::lock_guard<std::mutex> lock(mutex_);

      if (closed_)
      {
        return false;
      }

      task_ids_.push_back(id);
      ++total_tasks_;
      ++pending_tasks_;
      return true;
    }

    /**
     * @brief Mark one task as finished.
     *
     * This updates the group counters from a final task status/result and wakes
     * waiters when the group becomes idle.
     *
     * @param status Final task status.
     * @param result Final task result.
     * @param exception Optional exception captured by the task.
     */
    void finish_task(
        TaskStatus status,
        TaskResult result,
        std::exception_ptr exception = nullptr)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (pending_tasks_ > 0)
        {
          --pending_tasks_;
        }

        switch (status)
        {
        case TaskStatus::completed:
          ++completed_tasks_;
          break;

        case TaskStatus::failed:
          ++failed_tasks_;
          if (!first_exception_ && exception)
          {
            first_exception_ = exception;
          }
          break;

        case TaskStatus::cancelled:
          ++cancelled_tasks_;
          break;

        case TaskStatus::timed_out:
          ++timed_out_tasks_;
          break;

        case TaskStatus::rejected:
          ++rejected_tasks_;
          break;

        case TaskStatus::created:
        case TaskStatus::queued:
        case TaskStatus::running:
        default:
          if (result == TaskResult::success)
          {
            ++completed_tasks_;
          }
          else if (result == TaskResult::cancelled)
          {
            ++cancelled_tasks_;
          }
          else if (result == TaskResult::timeout)
          {
            ++timed_out_tasks_;
          }
          else if (result == TaskResult::rejected)
          {
            ++rejected_tasks_;
          }
          else if (result == TaskResult::failure)
          {
            ++failed_tasks_;
            if (!first_exception_ && exception)
            {
              first_exception_ = exception;
            }
          }
          break;
        }
      }

      cv_.notify_all();
    }

    /**
     * @brief Close the group against new task registrations.
     */
    void close()
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
      }

      cv_.notify_all();
    }

    /**
     * @brief Request cancellation for all tasks linked to this group.
     *
     * Cancellation is cooperative. Running tasks must observe their token.
     */
    void cancel() noexcept
    {
      cancellation_.request_cancel();
      cv_.notify_all();
    }

    /**
     * @brief Return a cancellation token shared by the group.
     *
     * @return Cancellation token.
     */
    [[nodiscard]] CancellationToken cancellation_token() const noexcept
    {
      return cancellation_.token();
    }

    /**
     * @brief Return the cancellation source used by this group.
     *
     * @return Cancellation source reference.
     */
    [[nodiscard]] CancellationSource &cancellation_source() noexcept
    {
      return cancellation_;
    }

    /**
     * @brief Return the cancellation source used by this group.
     *
     * @return Cancellation source reference.
     */
    [[nodiscard]] const CancellationSource &cancellation_source() const noexcept
    {
      return cancellation_;
    }

    /**
     * @brief Check whether group cancellation has been requested.
     *
     * @return true if cancellation was requested.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return cancellation_.cancelled();
    }

    /**
     * @brief Wait until all registered tasks finish.
     */
    void wait()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]()
               { return pending_tasks_ == 0; });
    }

    /**
     * @brief Wait until all registered tasks finish and rethrow first exception.
     *
     * If at least one task reported an exception, the first captured exception is
     * rethrown after all tasks finish.
     */
    void wait_and_rethrow()
    {
      std::exception_ptr exception;

      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]()
                 { return pending_tasks_ == 0; });

        exception = first_exception_;
      }

      if (exception)
      {
        std::rethrow_exception(exception);
      }
    }

    /**
     * @brief Check whether all registered tasks are finished.
     *
     * @return true if no task is pending.
     */
    [[nodiscard]] bool done() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return pending_tasks_ == 0;
    }

    /**
     * @brief Check whether the group is closed.
     *
     * @return true if no new tasks may be registered.
     */
    [[nodiscard]] bool closed() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return closed_;
    }

    /**
     * @brief Return whether no task is registered.
     *
     * @return true if the group is empty.
     */
    [[nodiscard]] bool empty() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return total_tasks_ == 0;
    }

    /**
     * @brief Return total number of registered tasks.
     *
     * @return Total task count.
     */
    [[nodiscard]] std::uint64_t total_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return total_tasks_;
    }

    /**
     * @brief Return number of tasks not finished yet.
     *
     * @return Pending task count.
     */
    [[nodiscard]] std::uint64_t pending_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return pending_tasks_;
    }

    /**
     * @brief Return number of tasks completed successfully.
     *
     * @return Completed task count.
     */
    [[nodiscard]] std::uint64_t completed_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return completed_tasks_;
    }

    /**
     * @brief Return number of failed tasks.
     *
     * @return Failed task count.
     */
    [[nodiscard]] std::uint64_t failed_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return failed_tasks_;
    }

    /**
     * @brief Return number of cancelled tasks.
     *
     * @return Cancelled task count.
     */
    [[nodiscard]] std::uint64_t cancelled_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return cancelled_tasks_;
    }

    /**
     * @brief Return number of timed out tasks.
     *
     * @return Timed out task count.
     */
    [[nodiscard]] std::uint64_t timed_out_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return timed_out_tasks_;
    }

    /**
     * @brief Return number of rejected tasks.
     *
     * @return Rejected task count.
     */
    [[nodiscard]] std::uint64_t rejected_tasks() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return rejected_tasks_;
    }

    /**
     * @brief Return whether at least one task failed.
     *
     * @return true if failed_tasks() is greater than zero.
     */
    [[nodiscard]] bool has_failure() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return failed_tasks_ > 0;
    }

    /**
     * @brief Return whether at least one task had a non-success outcome.
     *
     * @return true if the group contains failure, cancellation, timeout, or rejection.
     */
    [[nodiscard]] bool has_error() const
    {
      std::lock_guard<std::mutex> lock(mutex_);

      return failed_tasks_ > 0 ||
             cancelled_tasks_ > 0 ||
             timed_out_tasks_ > 0 ||
             rejected_tasks_ > 0;
    }

    /**
     * @brief Return a copy of all registered task ids.
     *
     * @return Task id list.
     */
    [[nodiscard]] std::vector<TaskId> task_ids() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return task_ids_;
    }

    /**
     * @brief Return the first exception captured by a failed task.
     *
     * @return Exception pointer, or nullptr.
     */
    [[nodiscard]] std::exception_ptr first_exception() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return first_exception_;
    }

  private:
    /**
     * @brief Registered task ids.
     */
    std::vector<TaskId> task_ids_;

    /**
     * @brief Shared cancellation source for the group.
     */
    CancellationSource cancellation_;

    /**
     * @brief Total number of registered tasks.
     */
    std::uint64_t total_tasks_;

    /**
     * @brief Number of tasks still pending.
     */
    std::uint64_t pending_tasks_;

    /**
     * @brief Number of completed tasks.
     */
    std::uint64_t completed_tasks_;

    /**
     * @brief Number of failed tasks.
     */
    std::uint64_t failed_tasks_;

    /**
     * @brief Number of cancelled tasks.
     */
    std::uint64_t cancelled_tasks_;

    /**
     * @brief Number of timed out tasks.
     */
    std::uint64_t timed_out_tasks_;

    /**
     * @brief Number of rejected tasks.
     */
    std::uint64_t rejected_tasks_;

    /**
     * @brief First exception captured by a failed task.
     */
    std::exception_ptr first_exception_;

    /**
     * @brief Whether the group is closed to new tasks.
     */
    bool closed_;

    /**
     * @brief Mutex protecting group state.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable used to wait for group completion.
     */
    mutable std::condition_variable cv_;
  };

  /**
   * @brief User-facing group of related threadpool tasks.
   *
   * TaskGroup provides structured coordination for several tasks submitted as
   * one logical unit. It can be used to:
   * - wait for all tasks
   * - request cancellation for all linked tasks
   * - inspect aggregate task outcomes
   * - rethrow the first captured exception after completion
   */
  class TaskGroup
  {
  public:
    /**
     * @brief Construct an empty task group.
     */
    TaskGroup()
        : state_()
    {
    }

    TaskGroup(const TaskGroup &) = delete;
    TaskGroup &operator=(const TaskGroup &) = delete;

    /**
     * @brief Move-construct a task group.
     */
    TaskGroup(TaskGroup &&) noexcept = delete;

    /**
     * @brief Move-assign a task group.
     *
     * @return Reference to this group.
     */
    TaskGroup &operator=(TaskGroup &&) noexcept = delete;

    /**
     * @brief Destroy the task group.
     */
    ~TaskGroup() = default;

    /**
     * @brief Register one task in the group.
     *
     * @param id Task identifier.
     * @return true if the task was registered.
     */
    [[nodiscard]] bool add_task(TaskId id)
    {
      return state_.add_task(id);
    }

    /**
     * @brief Mark one task as finished.
     *
     * @param status Final task status.
     * @param result Final task result.
     * @param exception Optional captured exception.
     */
    void finish_task(
        TaskStatus status,
        TaskResult result,
        std::exception_ptr exception = nullptr)
    {
      state_.finish_task(status, result, exception);
    }

    /**
     * @brief Close the group against new task registrations.
     */
    void close()
    {
      state_.close();
    }

    /**
     * @brief Request cancellation for all linked tasks.
     */
    void cancel() noexcept
    {
      state_.cancel();
    }

    /**
     * @brief Wait until all registered tasks finish.
     */
    void wait()
    {
      state_.wait();
    }

    /**
     * @brief Wait until all registered tasks finish and rethrow first exception.
     */
    void wait_and_rethrow()
    {
      state_.wait_and_rethrow();
    }

    /**
     * @brief Return a cancellation token shared by the group.
     *
     * @return Cancellation token.
     */
    [[nodiscard]] CancellationToken cancellation_token() const noexcept
    {
      return state_.cancellation_token();
    }

    /**
     * @brief Return the cancellation source used by this group.
     *
     * @return Cancellation source reference.
     */
    [[nodiscard]] CancellationSource &cancellation_source() noexcept
    {
      return state_.cancellation_source();
    }

    /**
     * @brief Check whether group cancellation has been requested.
     *
     * @return true if cancellation was requested.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return state_.cancelled();
    }

    /**
     * @brief Check whether all registered tasks are finished.
     *
     * @return true if no task is pending.
     */
    [[nodiscard]] bool done() const
    {
      return state_.done();
    }

    /**
     * @brief Check whether no task is registered.
     *
     * @return true if empty.
     */
    [[nodiscard]] bool empty() const
    {
      return state_.empty();
    }

    /**
     * @brief Check whether the group is closed.
     *
     * @return true if closed.
     */
    [[nodiscard]] bool closed() const
    {
      return state_.closed();
    }

    /**
     * @brief Return total registered task count.
     *
     * @return Total task count.
     */
    [[nodiscard]] std::uint64_t total_tasks() const
    {
      return state_.total_tasks();
    }

    /**
     * @brief Return pending task count.
     *
     * @return Pending task count.
     */
    [[nodiscard]] std::uint64_t pending_tasks() const
    {
      return state_.pending_tasks();
    }

    /**
     * @brief Return completed task count.
     *
     * @return Completed task count.
     */
    [[nodiscard]] std::uint64_t completed_tasks() const
    {
      return state_.completed_tasks();
    }

    /**
     * @brief Return failed task count.
     *
     * @return Failed task count.
     */
    [[nodiscard]] std::uint64_t failed_tasks() const
    {
      return state_.failed_tasks();
    }

    /**
     * @brief Return cancelled task count.
     *
     * @return Cancelled task count.
     */
    [[nodiscard]] std::uint64_t cancelled_tasks() const
    {
      return state_.cancelled_tasks();
    }

    /**
     * @brief Return timed out task count.
     *
     * @return Timed out task count.
     */
    [[nodiscard]] std::uint64_t timed_out_tasks() const
    {
      return state_.timed_out_tasks();
    }

    /**
     * @brief Return rejected task count.
     *
     * @return Rejected task count.
     */
    [[nodiscard]] std::uint64_t rejected_tasks() const
    {
      return state_.rejected_tasks();
    }

    /**
     * @brief Return whether at least one task failed.
     *
     * @return true if the group has failures.
     */
    [[nodiscard]] bool has_failure() const
    {
      return state_.has_failure();
    }

    /**
     * @brief Return whether at least one task had a non-success outcome.
     *
     * @return true if the group has any error-like outcome.
     */
    [[nodiscard]] bool has_error() const
    {
      return state_.has_error();
    }

    /**
     * @brief Return a copy of all registered task ids.
     *
     * @return Task id list.
     */
    [[nodiscard]] std::vector<TaskId> task_ids() const
    {
      return state_.task_ids();
    }

    /**
     * @brief Return the first captured exception.
     *
     * @return Exception pointer, or nullptr.
     */
    [[nodiscard]] std::exception_ptr first_exception() const
    {
      return state_.first_exception();
    }

  private:
    /**
     * @brief Internal shared group state.
     */
    TaskGroupState state_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_GROUP_HPP
