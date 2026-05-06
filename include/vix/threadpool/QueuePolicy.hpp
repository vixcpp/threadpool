/**
 *
 * @file QueuePolicy.hpp
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
#ifndef VIX_THREADPOOL_QUEUE_POLICY_HPP
#define VIX_THREADPOOL_QUEUE_POLICY_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Queue behavior used by the threadpool scheduler.
   *
   * QueuePolicy defines how tasks are stored and selected before execution.
   */
  enum class QueuePolicy : std::uint8_t
  {
    /**
     * @brief Use task priority first, then FIFO order for same-priority tasks.
     *
     * This is the default policy and uses TaskPriority plus sequence numbers.
     */
    priority = 0,

    /**
     * @brief Execute tasks in first-in-first-out order.
     *
     * This policy ignores priority and is useful for predictable simple queues.
     */
    fifo = 1,

    /**
     * @brief Execute most recently queued tasks first.
     *
     * This policy can improve locality for some workloads, but it is not fair
     * for long queues.
     */
    lifo = 2
  };

  /**
   * @brief Return the default queue policy.
   *
   * @return Default queue policy.
   */
  [[nodiscard]] inline constexpr QueuePolicy default_queue_policy() noexcept
  {
    return QueuePolicy::priority;
  }

  /**
   * @brief Check whether a queue policy uses task priorities.
   *
   * @param policy Queue policy.
   * @return true if priority affects task order.
   */
  [[nodiscard]] inline constexpr bool uses_priority(
      QueuePolicy policy) noexcept
  {
    return policy == QueuePolicy::priority;
  }

  /**
   * @brief Check whether a queue policy is FIFO.
   *
   * @param policy Queue policy.
   * @return true if the policy is FIFO.
   */
  [[nodiscard]] inline constexpr bool is_fifo(
      QueuePolicy policy) noexcept
  {
    return policy == QueuePolicy::fifo;
  }

  /**
   * @brief Check whether a queue policy is LIFO.
   *
   * @param policy Queue policy.
   * @return true if the policy is LIFO.
   */
  [[nodiscard]] inline constexpr bool is_lifo(
      QueuePolicy policy) noexcept
  {
    return policy == QueuePolicy::lifo;
  }

  /**
   * @brief Return a readable queue policy name.
   *
   * @param policy Queue policy.
   * @return Static policy name.
   */
  [[nodiscard]] inline constexpr const char *to_string(
      QueuePolicy policy) noexcept
  {
    switch (policy)
    {
    case QueuePolicy::priority:
      return "priority";
    case QueuePolicy::fifo:
      return "fifo";
    case QueuePolicy::lifo:
      return "lifo";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_QUEUE_POLICY_HPP
