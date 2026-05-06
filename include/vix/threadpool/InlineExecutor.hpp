/**
 *
 * @file InlineExecutor.hpp
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
#ifndef VIX_THREADPOOL_INLINE_EXECUTOR_HPP
#define VIX_THREADPOOL_INLINE_EXECUTOR_HPP

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <utility>

#include <vix/threadpool/Executor.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>

namespace vix::threadpool
{
  /**
   * @brief Executor that runs tasks immediately on the caller thread.
   *
   * InlineExecutor is useful for:
   * - tests
   * - deterministic execution
   * - fallback execution paths
   * - rejection policies such as caller-runs
   *
   * It does not create worker threads and does not queue tasks. A posted task is
   * executed synchronously inside post().
   */
  class InlineExecutor final : public Executor
  {
  public:
    /**
     * @brief Construct a running inline executor.
     */
    InlineExecutor() noexcept
        : running_(true),
          submitted_tasks_(0),
          completed_tasks_(0),
          failed_tasks_(0),
          cancelled_tasks_(0),
          timed_out_tasks_(0),
          rejected_tasks_(0),
          total_execution_time_(0),
          max_execution_time_(0)
    {
    }

    InlineExecutor(const InlineExecutor &) = delete;
    InlineExecutor &operator=(const InlineExecutor &) = delete;

    /**
     * @brief Move construction is disabled.
     */
    InlineExecutor(InlineExecutor &&) = delete;

    /**
     * @brief Move assignment is disabled.
     */
    InlineExecutor &operator=(InlineExecutor &&) = delete;

    /**
     * @brief Destroy the inline executor.
     */
    ~InlineExecutor() override = default;

    /**
     * @brief Run one task immediately on the caller thread.
     *
     * If the executor has been shut down and the task does not allow after-stop
     * execution, the task is rejected.
     *
     * @param task Callable to execute.
     * @param options Task submission options.
     * @return true if the task completed or was handled, false if rejected.
     */
    [[nodiscard]] bool post(
        Task task,
        TaskOptions options = TaskOptions{}) override
    {
      submitted_tasks_.fetch_add(1, std::memory_order_relaxed);

      if (!task)
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      if (!running_.load(std::memory_order_acquire) &&
          !options.allow_after_stop)
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      if (options.cancellation.cancelled())
      {
        cancelled_tasks_.fetch_add(1, std::memory_order_relaxed);
        return true;
      }

      if (options.deadline.expired())
      {
        timed_out_tasks_.fetch_add(1, std::memory_order_relaxed);
        return true;
      }

      const auto start = clock::now();

      try
      {
        task();

        const auto end = clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        record_execution_time(elapsed);

        if (options.timeout.expired(end - start) ||
            options.deadline.expired_at(end))
        {
          timed_out_tasks_.fetch_add(1, std::memory_order_relaxed);
          return true;
        }

        if (options.cancellation.cancelled())
        {
          cancelled_tasks_.fetch_add(1, std::memory_order_relaxed);
          return true;
        }

        completed_tasks_.fetch_add(1, std::memory_order_relaxed);
        return true;
      }
      catch (...)
      {
        const auto end = clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        record_execution_time(elapsed);
        failed_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }
    }

    /**
     * @brief Mark the executor as stopped.
     *
     * This operation is idempotent.
     */
    void shutdown() noexcept override
    {
      running_.store(false, std::memory_order_release);
    }

    /**
     * @brief Wait until the executor is idle.
     *
     * InlineExecutor is always idle after post() returns, so this function is a
     * no-op.
     */
    void wait_idle() override
    {
    }

    /**
     * @brief Check whether the executor accepts ordinary work.
     *
     * @return true if running.
     */
    [[nodiscard]] bool running() const noexcept override
    {
      return running_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check whether the executor is idle.
     *
     * InlineExecutor has no queue and no worker threads, so it is always idle.
     *
     * @return true.
     */
    [[nodiscard]] bool idle() const override
    {
      return true;
    }

    /**
     * @brief Return executor metrics.
     *
     * @return Metrics snapshot.
     */
    [[nodiscard]] ThreadPoolMetrics metrics() const override
    {
      ThreadPoolMetrics out;
      out.worker_count = 0;
      out.pending_tasks = 0;
      out.active_tasks = 0;
      out.idle_workers = 0;
      out.busy_workers = 0;
      out.submitted_tasks = submitted_tasks_.load(std::memory_order_relaxed);
      out.completed_tasks = completed_tasks_.load(std::memory_order_relaxed);
      out.failed_tasks = failed_tasks_.load(std::memory_order_relaxed);
      out.cancelled_tasks = cancelled_tasks_.load(std::memory_order_relaxed);
      out.timed_out_tasks = timed_out_tasks_.load(std::memory_order_relaxed);
      out.rejected_tasks = rejected_tasks_.load(std::memory_order_relaxed);
      return out;
    }

    /**
     * @brief Return cumulative executor stats.
     *
     * @return Stats snapshot.
     */
    [[nodiscard]] ThreadPoolStats stats() const override
    {
      ThreadPoolStats out;
      out.accepted_tasks =
          completed_tasks_.load(std::memory_order_relaxed) +
          failed_tasks_.load(std::memory_order_relaxed) +
          cancelled_tasks_.load(std::memory_order_relaxed) +
          timed_out_tasks_.load(std::memory_order_relaxed);
      out.rejected_tasks = rejected_tasks_.load(std::memory_order_relaxed);
      out.completed_tasks = completed_tasks_.load(std::memory_order_relaxed);
      out.failed_tasks = failed_tasks_.load(std::memory_order_relaxed);
      out.cancelled_tasks = cancelled_tasks_.load(std::memory_order_relaxed);
      out.timed_out_tasks = timed_out_tasks_.load(std::memory_order_relaxed);
      out.total_execution_time =
          std::chrono::nanoseconds{
              total_execution_time_.load(std::memory_order_relaxed)};
      out.max_execution_time =
          std::chrono::nanoseconds{
              max_execution_time_.load(std::memory_order_relaxed)};
      return out;
    }

  private:
    /**
     * @brief Clock used for execution timing.
     */
    using clock = std::chrono::steady_clock;

    /**
     * @brief Record one execution duration.
     *
     * @param elapsed Execution duration.
     */
    void record_execution_time(std::chrono::nanoseconds elapsed) noexcept
    {
      const auto value =
          static_cast<std::uint64_t>(elapsed.count() < 0 ? 0 : elapsed.count());

      total_execution_time_.fetch_add(value, std::memory_order_relaxed);

      std::uint64_t current = max_execution_time_.load(std::memory_order_relaxed);
      while (value > current &&
             !max_execution_time_.compare_exchange_weak(
                 current,
                 value,
                 std::memory_order_relaxed,
                 std::memory_order_relaxed))
      {
      }
    }

  private:
    /**
     * @brief Whether ordinary work is accepted.
     */
    std::atomic<bool> running_;

    /**
     * @brief Submitted task attempts.
     */
    std::atomic<std::uint64_t> submitted_tasks_;

    /**
     * @brief Completed task count.
     */
    std::atomic<std::uint64_t> completed_tasks_;

    /**
     * @brief Failed task count.
     */
    std::atomic<std::uint64_t> failed_tasks_;

    /**
     * @brief Cancelled task count.
     */
    std::atomic<std::uint64_t> cancelled_tasks_;

    /**
     * @brief Timed out task count.
     */
    std::atomic<std::uint64_t> timed_out_tasks_;

    /**
     * @brief Rejected task count.
     */
    std::atomic<std::uint64_t> rejected_tasks_;

    /**
     * @brief Total execution time in nanoseconds.
     */
    std::atomic<std::uint64_t> total_execution_time_;

    /**
     * @brief Maximum execution time in nanoseconds.
     */
    std::atomic<std::uint64_t> max_execution_time_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_INLINE_EXECUTOR_HPP
