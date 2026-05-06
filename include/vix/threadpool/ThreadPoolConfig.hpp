/**
 *
 * @file ThreadPoolConfig.hpp
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
#ifndef VIX_THREADPOOL_THREAD_POOL_CONFIG_HPP
#define VIX_THREADPOOL_THREAD_POOL_CONFIG_HPP

#include <chrono>
#include <cstddef>
#include <thread>

#include <vix/threadpool/TaskPriority.hpp>

namespace vix::threadpool
{
  /**
   * @brief Configuration used to create a ThreadPool.
   *
   * ThreadPoolConfig keeps the public pool configuration simple while still
   * allowing advanced behavior such as queue limits, dynamic growth, idle waits,
   * and default task options.
   */
  struct ThreadPoolConfig
  {
    /**
     * @brief Number of worker threads created when the pool starts.
     *
     * A value of 0 means hardware_concurrency() will be used.
     */
    std::size_t thread_count;

    /**
     * @brief Maximum number of worker threads allowed.
     *
     * If this value is lower than thread_count after normalization, it is raised
     * to match thread_count. A value of 0 means no explicit maximum was provided.
     */
    std::size_t max_thread_count;

    /**
     * @brief Maximum number of queued tasks.
     *
     * A value of 0 means the queue is unbounded.
     */
    std::size_t max_queue_size;

    /**
     * @brief Default priority used by task submission APIs.
     */
    TaskPriority default_priority;

    /**
     * @brief Whether workers may be created dynamically under backlog pressure.
     */
    bool allow_dynamic_growth;

    /**
     * @brief Whether the pool should finish queued tasks during shutdown.
     *
     * When true, shutdown drains queued work. When false, shutdown stops as soon
     * as possible and queued tasks may be rejected or discarded by higher layers.
     */
    bool drain_on_shutdown;

    /**
     * @brief Whether exceptions thrown by fire-and-forget tasks are swallowed.
     *
     * submit() tasks still propagate exceptions through their futures.
     */
    bool swallow_task_exceptions;

    /**
     * @brief Idle wait used by workers when no task is available.
     *
     * A zero duration means workers may use std::this_thread::yield().
     */
    std::chrono::microseconds idle_wait;

    /**
     * @brief Default timeout used for task execution observation.
     *
     * A zero duration disables timeout observation by default.
     */
    std::chrono::milliseconds default_timeout;

    /**
     * @brief Construct a default thread pool configuration.
     */
    ThreadPoolConfig() noexcept
        : thread_count(default_thread_count()),
          max_thread_count(default_thread_count()),
          max_queue_size(0),
          default_priority(TaskPriority::normal),
          allow_dynamic_growth(false),
          drain_on_shutdown(true),
          swallow_task_exceptions(true),
          idle_wait(std::chrono::microseconds{0}),
          default_timeout(std::chrono::milliseconds{0})
    {
    }

    /**
     * @brief Return a normalized copy of this configuration.
     *
     * Normalization guarantees:
     * - at least one worker thread
     * - max_thread_count is not lower than thread_count
     *
     * @return Normalized configuration.
     */
    [[nodiscard]] ThreadPoolConfig normalized() const noexcept
    {
      ThreadPoolConfig out = *this;

      if (out.thread_count == 0)
      {
        out.thread_count = default_thread_count();
      }

      if (out.thread_count == 0)
      {
        out.thread_count = 1;
      }

      if (out.max_thread_count == 0)
      {
        out.max_thread_count = out.thread_count;
      }

      if (out.max_thread_count < out.thread_count)
      {
        out.max_thread_count = out.thread_count;
      }

      return out;
    }

    /**
     * @brief Return the default worker count for the current machine.
     *
     * @return hardware_concurrency(), or 1 when unavailable.
     */
    [[nodiscard]] static std::size_t default_thread_count() noexcept
    {
      const unsigned int count = std::thread::hardware_concurrency();
      return count == 0u ? 1u : static_cast<std::size_t>(count);
    }
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_THREAD_POOL_CONFIG_HPP
