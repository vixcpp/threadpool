/**
 *
 * @file TaskPriority.hpp
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
#ifndef VIX_THREADPOOL_TASK_PRIORITY_HPP
#define VIX_THREADPOOL_TASK_PRIORITY_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Priority level assigned to a submitted task.
   *
   * Higher priority tasks are preferred by priority-based queues.
   * The values are intentionally stable and compact.
   */
  enum class TaskPriority : std::int32_t
  {
    /**
     * @brief Lowest priority task.
     */
    lowest = -2,

    /**
     * @brief Low priority task.
     */
    low = -1,

    /**
     * @brief Normal priority task.
     */
    normal = 0,

    /**
     * @brief High priority task.
     */
    high = 1,

    /**
     * @brief Highest priority task.
     */
    highest = 2
  };

  /**
   * @brief Convert a task priority to its numeric value.
   *
   * @param priority Task priority.
   * @return Numeric priority value.
   */
  [[nodiscard]] inline constexpr std::int32_t to_priority_value(TaskPriority priority) noexcept
  {
    return static_cast<std::int32_t>(priority);
  }

  /**
   * @brief Check whether one priority is higher than another.
   *
   * @param lhs Left priority.
   * @param rhs Right priority.
   * @return true if lhs is higher than rhs, false otherwise.
   */
  [[nodiscard]] inline constexpr bool priority_higher_than(
      TaskPriority lhs,
      TaskPriority rhs) noexcept
  {
    return to_priority_value(lhs) > to_priority_value(rhs);
  }

  /**
   * @brief Return a readable priority name.
   *
   * @param priority Task priority.
   * @return Static priority name.
   */
  [[nodiscard]] inline constexpr const char *to_string(TaskPriority priority) noexcept
  {
    switch (priority)
    {
    case TaskPriority::lowest:
      return "lowest";
    case TaskPriority::low:
      return "low";
    case TaskPriority::normal:
      return "normal";
    case TaskPriority::high:
      return "high";
    case TaskPriority::highest:
      return "highest";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_PRIORITY_HPP
