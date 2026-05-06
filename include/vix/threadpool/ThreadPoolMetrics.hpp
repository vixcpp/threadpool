/**
 *
 * @file ThreadPoolMetrics.hpp
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
#ifndef VIX_THREADPOOL_THREAD_POOL_METRICS_HPP
#define VIX_THREADPOOL_THREAD_POOL_METRICS_HPP

#include <cstdint>
#include <cstddef>

namespace vix::threadpool
{
  /**
   * @brief Lightweight snapshot of the current thread pool state.
   *
   * ThreadPoolMetrics describes what is happening inside the pool right now.
   * It is intended for dashboards, logs, tests, and runtime diagnostics.
   */
  struct ThreadPoolMetrics
  {
    /**
     * @brief Number of worker threads currently owned by the pool.
     */
    std::size_t worker_count;

    /**
     * @brief Number of tasks currently waiting in the queue.
     */
    std::size_t pending_tasks;

    /**
     * @brief Number of tasks currently being executed.
     */
    std::uint64_t active_tasks;

    /**
     * @brief Number of workers currently idle.
     */
    std::size_t idle_workers;

    /**
     * @brief Number of workers currently executing work.
     */
    std::size_t busy_workers;

    /**
     * @brief Total number of tasks submitted since pool creation.
     */
    std::uint64_t submitted_tasks;

    /**
     * @brief Total number of tasks completed successfully.
     */
    std::uint64_t completed_tasks;

    /**
     * @brief Total number of tasks failed with an exception or execution error.
     */
    std::uint64_t failed_tasks;

    /**
     * @brief Total number of tasks cancelled before or during execution.
     */
    std::uint64_t cancelled_tasks;

    /**
     * @brief Total number of tasks observed as timed out.
     */
    std::uint64_t timed_out_tasks;

    /**
     * @brief Total number of tasks rejected before entering the queue.
     */
    std::uint64_t rejected_tasks;

    /**
     * @brief Construct an empty metrics snapshot.
     */
    constexpr ThreadPoolMetrics() noexcept
        : worker_count(0),
          pending_tasks(0),
          active_tasks(0),
          idle_workers(0),
          busy_workers(0),
          submitted_tasks(0),
          completed_tasks(0),
          failed_tasks(0),
          cancelled_tasks(0),
          timed_out_tasks(0),
          rejected_tasks(0)
    {
    }

    /**
     * @brief Check whether the pool is currently idle.
     *
     * @return true if no task is pending or active, false otherwise.
     */
    [[nodiscard]] constexpr bool idle() const noexcept
    {
      return pending_tasks == 0 && active_tasks == 0;
    }

    /**
     * @brief Return total finished tasks.
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
     * @brief Return total non-successful tasks.
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
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_THREAD_POOL_METRICS_HPP
