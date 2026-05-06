/**
 *
 * @file RejectionPolicy.hpp
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
#ifndef VIX_THREADPOOL_REJECTION_POLICY_HPP
#define VIX_THREADPOOL_REJECTION_POLICY_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Policy used when a task cannot be accepted by the thread pool.
   *
   * RejectionPolicy controls what happens when task submission fails because the
   * pool is stopped, the queue is full, or the scheduler cannot select a worker.
   */
  enum class RejectionPolicy : std::uint8_t
  {
    /**
     * @brief Reject the task immediately.
     *
     * This is the safest default because submission never blocks and rejected
     * tasks can be reported clearly to the caller.
     */
    reject = 0,

    /**
     * @brief Run the task immediately on the caller thread.
     *
     * This can reduce task loss under pressure, but it may block the submitting
     * thread and should be used only when that behavior is acceptable.
     */
    caller_runs = 1,

    /**
     * @brief Silently discard the task.
     *
     * This is useful only for best-effort background work such as telemetry.
     */
    discard = 2
  };

  /**
   * @brief Return the default rejection policy.
   *
   * @return Default rejection policy.
   */
  [[nodiscard]] inline constexpr RejectionPolicy default_rejection_policy() noexcept
  {
    return RejectionPolicy::reject;
  }

  /**
   * @brief Check whether rejected tasks should run on the caller thread.
   *
   * @param policy Rejection policy.
   * @return true if rejected tasks should run on the caller thread.
   */
  [[nodiscard]] inline constexpr bool runs_on_caller(
      RejectionPolicy policy) noexcept
  {
    return policy == RejectionPolicy::caller_runs;
  }

  /**
   * @brief Check whether rejected tasks should be discarded.
   *
   * @param policy Rejection policy.
   * @return true if rejected tasks should be discarded silently.
   */
  [[nodiscard]] inline constexpr bool discards_task(
      RejectionPolicy policy) noexcept
  {
    return policy == RejectionPolicy::discard;
  }

  /**
   * @brief Check whether rejected tasks should be reported as rejected.
   *
   * @param policy Rejection policy.
   * @return true if rejected tasks should be reported.
   */
  [[nodiscard]] inline constexpr bool reports_rejection(
      RejectionPolicy policy) noexcept
  {
    return policy == RejectionPolicy::reject;
  }

  /**
   * @brief Return a readable rejection policy name.
   *
   * @param policy Rejection policy.
   * @return Static policy name.
   */
  [[nodiscard]] inline constexpr const char *to_string(
      RejectionPolicy policy) noexcept
  {
    switch (policy)
    {
    case RejectionPolicy::reject:
      return "reject";
    case RejectionPolicy::caller_runs:
      return "caller_runs";
    case RejectionPolicy::discard:
      return "discard";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_REJECTION_POLICY_HPP
