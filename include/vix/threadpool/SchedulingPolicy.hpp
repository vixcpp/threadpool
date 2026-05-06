/**
 *
 * @file SchedulingPolicy.hpp
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
#ifndef VIX_THREADPOOL_SCHEDULING_POLICY_HPP
#define VIX_THREADPOOL_SCHEDULING_POLICY_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Scheduling strategy used to choose a worker for a submitted task.
   *
   * SchedulingPolicy controls how the scheduler distributes accepted tasks
   * across worker queues.
   */
  enum class SchedulingPolicy : std::uint8_t
  {
    /**
     * @brief Submit tasks to workers in round-robin order.
     *
     * This is simple, predictable, and usually a good default.
     */
    round_robin = 0,

    /**
     * @brief Prefer the worker with the smallest queue.
     *
     * This can improve balance when task durations vary.
     */
    least_loaded = 1,

    /**
     * @brief Prefer the task affinity hint when present.
     *
     * If no valid affinity is configured, the scheduler falls back to
     * round-robin behavior.
     */
    affinity = 2,

    /**
     * @brief Prefer affinity first, then choose the least loaded worker.
     *
     * This is useful when some tasks have locality hints but most tasks do not.
     */
    affinity_then_least_loaded = 3
  };

  /**
   * @brief Return the default scheduling policy.
   *
   * @return Default scheduling policy.
   */
  [[nodiscard]] inline constexpr SchedulingPolicy default_scheduling_policy() noexcept
  {
    return SchedulingPolicy::affinity_then_least_loaded;
  }

  /**
   * @brief Check whether a scheduling policy uses worker affinity.
   *
   * @param policy Scheduling policy.
   * @return true if the policy uses affinity hints.
   */
  [[nodiscard]] inline constexpr bool uses_affinity(
      SchedulingPolicy policy) noexcept
  {
    return policy == SchedulingPolicy::affinity ||
           policy == SchedulingPolicy::affinity_then_least_loaded;
  }

  /**
   * @brief Check whether a scheduling policy uses load information.
   *
   * @param policy Scheduling policy.
   * @return true if the policy prefers the least loaded worker.
   */
  [[nodiscard]] inline constexpr bool uses_load_balancing(
      SchedulingPolicy policy) noexcept
  {
    return policy == SchedulingPolicy::least_loaded ||
           policy == SchedulingPolicy::affinity_then_least_loaded;
  }

  /**
   * @brief Return a readable scheduling policy name.
   *
   * @param policy Scheduling policy.
   * @return Static policy name.
   */
  [[nodiscard]] inline constexpr const char *to_string(
      SchedulingPolicy policy) noexcept
  {
    switch (policy)
    {
    case SchedulingPolicy::round_robin:
      return "round_robin";
    case SchedulingPolicy::least_loaded:
      return "least_loaded";
    case SchedulingPolicy::affinity:
      return "affinity";
    case SchedulingPolicy::affinity_then_least_loaded:
      return "affinity_then_least_loaded";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_SCHEDULING_POLICY_HPP
