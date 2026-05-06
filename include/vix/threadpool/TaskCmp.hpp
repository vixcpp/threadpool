/**
 *
 * @file TaskCmp.hpp
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
#ifndef VIX_THREADPOOL_TASK_CMP_HPP
#define VIX_THREADPOOL_TASK_CMP_HPP

#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskPriority.hpp>

namespace vix::threadpool
{
  /**
   * @brief Comparator used by priority-based task queues.
   *
   * TaskCmp orders tasks by priority first, then by sequence number.
   *
   * Ordering rules:
   * - higher priority tasks run before lower priority tasks
   * - tasks with the same priority keep FIFO order using their sequence number
   *
   * This comparator is designed for std::priority_queue, where returning true
   * means the left task has lower priority than the right task.
   */
  struct TaskCmp
  {
    /**
     * @brief Compare two tasks for priority queue ordering.
     *
     * @param lhs Left task.
     * @param rhs Right task.
     * @return true if lhs should be ordered after rhs.
     */
    [[nodiscard]] bool operator()(const Task &lhs, const Task &rhs) const noexcept
    {
      const std::int32_t lhsPriority = to_priority_value(lhs.priority());
      const std::int32_t rhsPriority = to_priority_value(rhs.priority());

      if (lhsPriority != rhsPriority)
      {
        return lhsPriority < rhsPriority;
      }

      return lhs.sequence() > rhs.sequence();
    }
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_CMP_HPP
