/**
 *
 * @file TaskOptions.hpp
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
#ifndef VIX_THREADPOOL_TASK_OPTIONS_HPP
#define VIX_THREADPOOL_TASK_OPTIONS_HPP

#include <cstdint>

#include <vix/threadpool/CancellationToken.hpp>
#include <vix/threadpool/Deadline.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/Timeout.hpp>
#include <vix/threadpool/WorkerId.hpp>

namespace vix::threadpool
{
  /**
   * @brief Options used when submitting a task to the thread pool.
   *
   * TaskOptions keeps task configuration explicit and lightweight.
   *
   * It controls:
   * - priority
   * - timeout observation
   * - absolute deadline
   * - cooperative cancellation
   * - optional worker affinity
   *
   * These options do not forcefully stop a running C++ function. Cancellation,
   * timeout, and deadline are cooperative or observational unless the task body
   * checks the provided cancellation token.
   */
  struct TaskOptions
  {
    /**
     * @brief Priority used by priority-aware task queues.
     */
    TaskPriority priority;

    /**
     * @brief Timeout used to observe task execution duration.
     *
     * A disabled timeout means no timeout observation is applied.
     */
    Timeout timeout;

    /**
     * @brief Absolute deadline for the task.
     *
     * A disabled deadline means no absolute deadline is applied.
     */
    Deadline deadline;

    /**
     * @brief Cancellation token observed before task execution.
     */
    CancellationToken cancellation;

    /**
     * @brief Optional worker affinity hint.
     *
     * A value of invalid_worker_id means no worker preference.
     */
    WorkerId affinity;

    /**
     * @brief Whether this task may be executed after shutdown begins.
     *
     * Most user tasks should keep the default false value.
     */
    bool allow_after_stop;

    /**
     * @brief Whether this task is fire-and-forget.
     *
     * Fire-and-forget tasks may swallow exceptions depending on pool config.
     */
    bool detached;

    /**
     * @brief User-defined flags reserved for higher-level integrations.
     */
    std::uint32_t flags;

    /**
     * @brief Construct default task options.
     */
    TaskOptions() noexcept
        : priority(TaskPriority::normal),
          timeout(Timeout::disabled()),
          deadline(Deadline::disabled()),
          cancellation(),
          affinity(invalid_worker_id),
          allow_after_stop(false),
          detached(false),
          flags(0)
    {
    }

    /**
     * @brief Create task options with a specific priority.
     *
     * @param value Task priority.
     * @return Configured task options.
     */
    [[nodiscard]] static TaskOptions with_priority(TaskPriority value) noexcept
    {
      TaskOptions options;
      options.priority = value;
      return options;
    }

    /**
     * @brief Create task options with a timeout.
     *
     * @param value Timeout value.
     * @return Configured task options.
     */
    [[nodiscard]] static TaskOptions with_timeout(Timeout value) noexcept
    {
      TaskOptions options;
      options.timeout = value;
      return options;
    }

    /**
     * @brief Create task options with a deadline.
     *
     * @param value Deadline value.
     * @return Configured task options.
     */
    [[nodiscard]] static TaskOptions with_deadline(Deadline value) noexcept
    {
      TaskOptions options;
      options.deadline = value;
      return options;
    }

    /**
     * @brief Create task options with a cancellation token.
     *
     * @param value Cancellation token.
     * @return Configured task options.
     */
    [[nodiscard]] static TaskOptions with_cancellation(CancellationToken value) noexcept
    {
      TaskOptions options;
      options.cancellation = std::move(value);
      return options;
    }

    /**
     * @brief Create task options with a worker affinity hint.
     *
     * @param value Worker affinity.
     * @return Configured task options.
     */
    [[nodiscard]] static TaskOptions with_affinity(WorkerId value) noexcept
    {
      TaskOptions options;
      options.affinity = value;
      return options;
    }

    /**
     * @brief Check whether this task has a worker affinity hint.
     *
     * @return true if a valid worker id was configured.
     */
    [[nodiscard]] bool has_affinity() const noexcept
    {
      return is_valid_worker_id(affinity);
    }

    /**
     * @brief Check whether this task has timeout observation enabled.
     *
     * @return true if timeout is enabled.
     */
    [[nodiscard]] bool has_timeout() const noexcept
    {
      return timeout.enabled();
    }

    /**
     * @brief Check whether this task has an absolute deadline.
     *
     * @return true if deadline is enabled.
     */
    [[nodiscard]] bool has_deadline() const noexcept
    {
      return deadline.enabled();
    }

    /**
     * @brief Check whether this task can observe cancellation.
     *
     * @return true if the cancellation token is connected to a source.
     */
    [[nodiscard]] bool has_cancellation() const noexcept
    {
      return cancellation.can_cancel();
    }

    /**
     * @brief Check whether the task should be skipped before execution.
     *
     * @return true if cancellation was requested or the deadline already expired.
     */
    [[nodiscard]] bool should_skip_before_run() const noexcept
    {
      return cancellation.cancelled() || deadline.expired();
    }

    /**
     * @brief Set the task priority.
     *
     * @param value New priority.
     * @return Reference to this options object.
     */
    TaskOptions &set_priority(TaskPriority value) noexcept
    {
      priority = value;
      return *this;
    }

    /**
     * @brief Set the task timeout.
     *
     * @param value New timeout.
     * @return Reference to this options object.
     */
    TaskOptions &set_timeout(Timeout value) noexcept
    {
      timeout = value;
      return *this;
    }

    /**
     * @brief Set the task deadline.
     *
     * @param value New deadline.
     * @return Reference to this options object.
     */
    TaskOptions &set_deadline(Deadline value) noexcept
    {
      deadline = value;
      return *this;
    }

    /**
     * @brief Set the task cancellation token.
     *
     * @param value New cancellation token.
     * @return Reference to this options object.
     */
    TaskOptions &set_cancellation(CancellationToken value) noexcept
    {
      cancellation = std::move(value);
      return *this;
    }

    /**
     * @brief Set the task worker affinity.
     *
     * @param value Worker id affinity.
     * @return Reference to this options object.
     */
    TaskOptions &set_affinity(WorkerId value) noexcept
    {
      affinity = value;
      return *this;
    }

    /**
     * @brief Mark the task as detached or non-detached.
     *
     * @param value Detached flag.
     * @return Reference to this options object.
     */
    TaskOptions &set_detached(bool value) noexcept
    {
      detached = value;
      return *this;
    }

    /**
     * @brief Allow or deny execution after shutdown begins.
     *
     * @param value Allow-after-stop flag.
     * @return Reference to this options object.
     */
    TaskOptions &set_allow_after_stop(bool value) noexcept
    {
      allow_after_stop = value;
      return *this;
    }
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_OPTIONS_HPP
