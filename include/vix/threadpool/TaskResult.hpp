/**
 *
 * @file TaskResult.hpp
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
#ifndef VIX_THREADPOOL_TASK_RESULT_HPP
#define VIX_THREADPOOL_TASK_RESULT_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Result of a task execution attempt.
   *
   * TaskResult is used internally and by low-level APIs to describe how a task
   * execution finished.
   */
  enum class TaskResult : std::uint8_t
  {
    /**
     * @brief Task has not produced a result yet.
     */
    none = 0,

    /**
     * @brief Task completed successfully.
     */
    success = 1,

    /**
     * @brief Task failed during execution.
     */
    failure = 2,

    /**
     * @brief Task was cancelled.
     */
    cancelled = 3,

    /**
     * @brief Task timed out.
     */
    timeout = 4,

    /**
     * @brief Task was rejected before execution.
     */
    rejected = 5
  };

  /**
   * @brief Check whether a task result represents success.
   *
   * @param result Task result.
   * @return true if the result is success, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_success(TaskResult result) noexcept
  {
    return result == TaskResult::success;
  }

  /**
   * @brief Check whether a task result represents failure.
   *
   * @param result Task result.
   * @return true if the result is a non-success final result, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_failure(TaskResult result) noexcept
  {
    return result == TaskResult::failure ||
           result == TaskResult::cancelled ||
           result == TaskResult::timeout ||
           result == TaskResult::rejected;
  }

  /**
   * @brief Return a readable task result name.
   *
   * @param result Task result.
   * @return Static result name.
   */
  [[nodiscard]] inline constexpr const char *to_string(TaskResult result) noexcept
  {
    switch (result)
    {
    case TaskResult::none:
      return "none";
    case TaskResult::success:
      return "success";
    case TaskResult::failure:
      return "failure";
    case TaskResult::cancelled:
      return "cancelled";
    case TaskResult::timeout:
      return "timeout";
    case TaskResult::rejected:
      return "rejected";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_RESULT_HPP
