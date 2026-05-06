/**
 *
 * @file TaskStatus.hpp
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
#ifndef VIX_THREADPOOL_TASK_STATUS_HPP
#define VIX_THREADPOOL_TASK_STATUS_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Lifecycle status of a task inside the thread pool.
   *
   * TaskStatus describes where a task currently is in the execution lifecycle.
   */
  enum class TaskStatus : std::uint8_t
  {
    /**
     * @brief Task has been created but not submitted yet.
     */
    created = 0,

    /**
     * @brief Task has been accepted and is waiting in a queue.
     */
    queued = 1,

    /**
     * @brief Task is currently being executed by a worker.
     */
    running = 2,

    /**
     * @brief Task completed successfully.
     */
    completed = 3,

    /**
     * @brief Task failed with an exception or execution error.
     */
    failed = 4,

    /**
     * @brief Task was cancelled before or during execution.
     */
    cancelled = 5,

    /**
     * @brief Task exceeded its configured timeout or deadline.
     */
    timed_out = 6,

    /**
     * @brief Task was rejected by the pool before entering the queue.
     */
    rejected = 7
  };

  /**
   * @brief Check whether a task status is terminal.
   *
   * @param status Task status to inspect.
   * @return true if the task reached a final state, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_terminal(TaskStatus status) noexcept
  {
    return status == TaskStatus::completed ||
           status == TaskStatus::failed ||
           status == TaskStatus::cancelled ||
           status == TaskStatus::timed_out ||
           status == TaskStatus::rejected;
  }

  /**
   * @brief Check whether a task status represents active work.
   *
   * @param status Task status to inspect.
   * @return true if the task is queued or running, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_active(TaskStatus status) noexcept
  {
    return status == TaskStatus::queued ||
           status == TaskStatus::running;
  }

  /**
   * @brief Return a readable task status name.
   *
   * @param status Task status.
   * @return Static status name.
   */
  [[nodiscard]] inline constexpr const char *to_string(TaskStatus status) noexcept
  {
    switch (status)
    {
    case TaskStatus::created:
      return "created";
    case TaskStatus::queued:
      return "queued";
    case TaskStatus::running:
      return "running";
    case TaskStatus::completed:
      return "completed";
    case TaskStatus::failed:
      return "failed";
    case TaskStatus::cancelled:
      return "cancelled";
    case TaskStatus::timed_out:
      return "timed_out";
    case TaskStatus::rejected:
      return "rejected";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_STATUS_HPP
