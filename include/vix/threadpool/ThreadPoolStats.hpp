/**
 *
 * @file ThreadPoolStats.hpp
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
#ifndef VIX_THREADPOOL_THREAD_POOL_STATS_HPP
#define VIX_THREADPOOL_THREAD_POOL_STATS_HPP

#include <chrono>
#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Cumulative statistics collected by a thread pool.
   *
   * ThreadPoolStats is different from ThreadPoolMetrics:
   * - metrics describe the current state
   * - stats describe historical counters and timing information
   */
  struct ThreadPoolStats
  {
    /**
     * @brief Total number of accepted tasks.
     */
    std::uint64_t accepted_tasks;

    /**
     * @brief Total number of rejected tasks.
     */
    std::uint64_t rejected_tasks;

    /**
     * @brief Total number of completed tasks.
     */
    std::uint64_t completed_tasks;

    /**
     * @brief Total number of failed tasks.
     */
    std::uint64_t failed_tasks;

    /**
     * @brief Total number of cancelled tasks.
     */
    std::uint64_t cancelled_tasks;

    /**
     * @brief Total number of timed out tasks.
     */
    std::uint64_t timed_out_tasks;

    /**
     * @brief Total number of worker wakeups.
     */
    std::uint64_t worker_wakeups;

    /**
     * @brief Total number of idle worker waits.
     */
    std::uint64_t idle_waits;

    /**
     * @brief Total time spent executing tasks.
     */
    std::chrono::nanoseconds total_execution_time;

    /**
     * @brief Longest observed task execution time.
     */
    std::chrono::nanoseconds max_execution_time;

    /**
     * @brief Construct empty cumulative stats.
     */
    constexpr ThreadPoolStats() noexcept
        : accepted_tasks(0),
          rejected_tasks(0),
          completed_tasks(0),
          failed_tasks(0),
          cancelled_tasks(0),
          timed_out_tasks(0),
          worker_wakeups(0),
          idle_waits(0),
          total_execution_time(std::chrono::nanoseconds{0}),
          max_execution_time(std::chrono::nanoseconds{0})
    {
    }

    /**
     * @brief Return total submitted task attempts.
     *
     * @return accepted + rejected task count.
     */
    [[nodiscard]] constexpr std::uint64_t submitted_tasks() const noexcept
    {
      return accepted_tasks + rejected_tasks;
    }

    /**
     * @brief Return total finished accepted tasks.
     *
     * @return completed + failed + cancelled + timed out task count.
     */
    [[nodiscard]] constexpr std::uint64_t finished_tasks() const noexcept
    {
      return completed_tasks +
             failed_tasks +
             cancelled_tasks +
             timed_out_tasks;
    }

    /**
     * @brief Return total non-successful task outcomes.
     *
     * @return failed + cancelled + timed out + rejected task count.
     */
    [[nodiscard]] constexpr std::uint64_t error_tasks() const noexcept
    {
      return failed_tasks +
             cancelled_tasks +
             timed_out_tasks +
             rejected_tasks;
    }

    /**
     * @brief Return whether no task has been submitted yet.
     *
     * @return true if no accepted or rejected task exists, false otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return submitted_tasks() == 0;
    }

    /**
     * @brief Return the average execution time per completed task.
     *
     * Only successfully completed tasks are used for the divisor.
     *
     * @return Average execution duration, or zero when no task completed.
     */
    [[nodiscard]] constexpr std::chrono::nanoseconds average_execution_time() const noexcept
    {
      if (completed_tasks == 0)
      {
        return std::chrono::nanoseconds{0};
      }

      return total_execution_time / completed_tasks;
    }
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_THREAD_POOL_STATS_HPP
