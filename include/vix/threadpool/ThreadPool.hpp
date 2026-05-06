/**
 *
 * @file ThreadPool.hpp
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
#ifndef VIX_THREADPOOL_THREAD_POOL_HPP
#define VIX_THREADPOOL_THREAD_POOL_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <vix/threadpool/Executor.hpp>
#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/PeriodicTask.hpp>
#include <vix/threadpool/Promise.hpp>
#include <vix/threadpool/Scheduler.hpp>
#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskGroup.hpp>
#include <vix/threadpool/TaskHandle.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPoolConfig.hpp>
#include <vix/threadpool/ThreadPoolError.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>
#include <vix/threadpool/detail/Invoke.hpp>

namespace vix::threadpool
{
  /**
   * @brief Main public thread pool API.
   *
   * ThreadPool is the high-level entry point for running work concurrently.
   *
   * It provides:
   * - post() for fire-and-forget work
   * - submit() for future-returning work
   * - handle() for cancellable user-facing task handles
   * - schedule_every() for periodic task submission
   * - metrics() and stats() for observability
   *
   * The class owns a Scheduler, which owns the actual workers.
   */
  class ThreadPool final : public Executor
  {
  public:
    /**
     * @brief Construct a thread pool using default configuration.
     *
     * The pool starts automatically.
     */
    ThreadPool()
        : ThreadPool(ThreadPoolConfig{})
    {
    }

    /**
     * @brief Construct a thread pool with a fixed worker count.
     *
     * The pool starts automatically.
     *
     * @param threads Number of worker threads.
     */
    explicit ThreadPool(std::size_t threads)
        : ThreadPool(make_config_from_thread_count(threads))
    {
    }

    /**
     * @brief Construct a thread pool from configuration.
     *
     * The pool starts automatically.
     *
     * @param config Thread pool configuration.
     */
    explicit ThreadPool(ThreadPoolConfig config)
        : config_(config.normalized()),
          scheduler_(make_scheduler_config(config_)),
          next_task_id_(1),
          next_sequence_(0),
          running_(false)
    {
      start();
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    /**
     * @brief Shut down the pool on destruction.
     */
    ~ThreadPool() noexcept override
    {
      shutdown();
    }

    /**
     * @brief Start the pool if it is not already running.
     *
     * @return true if the pool transitioned to running.
     */
    [[nodiscard]] bool start()
    {
      bool expected = false;
      if (!running_.compare_exchange_strong(
              expected,
              true,
              std::memory_order_acq_rel,
              std::memory_order_acquire))
      {
        return false;
      }

      const bool ok = scheduler_.start();
      if (!ok)
      {
        running_.store(false, std::memory_order_release);
      }

      return ok;
    }

    /**
     * @brief Post one fire-and-forget task.
     *
     * The task is submitted for background execution. Exceptions are captured by
     * the internal task object and do not escape worker threads.
     *
     * @param task Callable to execute.
     * @param options Task submission options.
     * @return true if the task was accepted or handled successfully.
     */
    [[nodiscard]] bool post(
        Executor::Task task,
        TaskOptions options = TaskOptions{}) override
    {
      if (!task)
      {
        return false;
      }

      options.detached = true;

      vix::threadpool::Task wrapped(
          next_task_id(),
          TaskFunction(std::move(task)),
          merge_options(std::move(options)),
          next_sequence());

      return scheduler_.submit(std::move(wrapped));
    }

    /**
     * @brief Submit a callable and return a Future for its result.
     *
     * @tparam Fn Callable type.
     * @param fn Callable to execute.
     * @param options Task submission options.
     * @return Future producing the callable result.
     */
    template <class Fn>
    [[nodiscard]] auto submit(
        Fn &&fn,
        TaskOptions options = TaskOptions{})
        -> Future<std::invoke_result_t<std::decay_t<Fn> &>>
    {
      using Function = std::decay_t<Fn>;
      using Result = std::invoke_result_t<Function &>;

      Promise<Result> promise;
      Future<Result> future = promise.get_future();

      auto sharedPromise =
          std::make_shared<Promise<Result>>(std::move(promise));

      Function function(std::forward<Fn>(fn));

      auto wrapper =
          [sharedPromise, function = std::move(function)]() mutable
      {
        try
        {
          if constexpr (std::is_void_v<Result>)
          {
            detail::invoke(function);
            sharedPromise->set_value();
          }
          else
          {
            sharedPromise->set_value(detail::invoke(function));
          }
        }
        catch (...)
        {
          sharedPromise->set_current_exception();
        }
      };

      vix::threadpool::Task task(
          next_task_id(),
          TaskFunction(std::move(wrapper)),
          merge_options(std::move(options)),
          next_sequence());

      const bool accepted = scheduler_.submit(std::move(task));
      if (!accepted)
      {
        sharedPromise->set_error(ThreadPoolErrc::rejected);
      }

      return future;
    }

    /**
     * @brief Submit a callable and return a cancellable task handle.
     *
     * The returned handle contains both the future and a cancellation source.
     *
     * @tparam Fn Callable type.
     * @param fn Callable to execute.
     * @param options Task submission options.
     * @return Task handle for the submitted task.
     */
    template <class Fn>
    [[nodiscard]] auto handle(
        Fn &&fn,
        TaskOptions options = TaskOptions{})
        -> TaskHandle<std::invoke_result_t<std::decay_t<Fn> &>>
    {
      using Function = std::decay_t<Fn>;
      using Result = std::invoke_result_t<Function &>;

      CancellationSource source;
      options.set_cancellation(source.token());

      const TaskId id = next_task_id();

      Promise<Result> promise;
      Future<Result> future = promise.get_future();

      auto sharedPromise =
          std::make_shared<Promise<Result>>(std::move(promise));

      Function function(std::forward<Fn>(fn));

      auto wrapper =
          [sharedPromise, function = std::move(function)]() mutable
      {
        try
        {
          if constexpr (std::is_void_v<Result>)
          {
            detail::invoke(function);
            sharedPromise->set_value();
          }
          else
          {
            sharedPromise->set_value(detail::invoke(function));
          }
        }
        catch (...)
        {
          sharedPromise->set_current_exception();
        }
      };

      vix::threadpool::Task task(
          id,
          TaskFunction(std::move(wrapper)),
          merge_options(std::move(options)),
          next_sequence());

      const bool accepted = scheduler_.submit(std::move(task));
      if (!accepted)
      {
        sharedPromise->set_error(ThreadPoolErrc::rejected);
      }

      return TaskHandle<Result>{id, std::move(future), std::move(source)};
    }

    /**
     * @brief Create a periodic task bound to this pool.
     *
     * The returned PeriodicTask is not started automatically.
     *
     * @param callback Callback submitted at each interval.
     * @param config Periodic task configuration.
     * @return Periodic task object.
     */
    [[nodiscard]] PeriodicTask schedule_every(
        PeriodicTask::Callback callback,
        PeriodicTaskConfig config = PeriodicTaskConfig{})
    {
      return PeriodicTask(*this, std::move(callback), config);
    }

    /**
     * @brief Request pool shutdown.
     *
     * This operation is safe and idempotent.
     */
    void shutdown() noexcept override
    {
      running_.store(false, std::memory_order_release);
      scheduler_.stop();
      scheduler_.join();
    }

    /**
     * @brief Wait until the pool has no pending or active work.
     */
    void wait_idle() override
    {
      while (!idle())
      {
        std::this_thread::yield();
      }
    }

    /**
     * @brief Check whether the pool accepts ordinary work.
     *
     * @return true if running.
     */
    [[nodiscard]] bool running() const noexcept override
    {
      return running_.load(std::memory_order_acquire) &&
             scheduler_.running();
    }

    /**
     * @brief Check whether no work is pending or active.
     *
     * @return true if the pool is idle.
     */
    [[nodiscard]] bool idle() const override
    {
      return metrics().idle();
    }

    /**
     * @brief Return current pool metrics.
     *
     * @return Metrics snapshot.
     */
    [[nodiscard]] ThreadPoolMetrics metrics() const override
    {
      return scheduler_.metrics();
    }

    /**
     * @brief Return cumulative pool stats.
     *
     * @return Stats snapshot.
     */
    [[nodiscard]] ThreadPoolStats stats() const override
    {
      return scheduler_.stats();
    }

    /**
     * @brief Return the thread pool configuration.
     *
     * @return Configuration.
     */
    [[nodiscard]] const ThreadPoolConfig &config() const noexcept
    {
      return config_;
    }

    /**
     * @brief Return the number of worker threads.
     *
     * @return Worker count.
     */
    [[nodiscard]] std::size_t thread_count() const noexcept
    {
      return scheduler_.worker_count();
    }

    /**
     * @brief Return the total number of queued tasks.
     *
     * @return Pending task count.
     */
    [[nodiscard]] std::size_t pending() const
    {
      return scheduler_.size();
    }

    /**
     * @brief Clear queued tasks that have not started yet.
     *
     * @return Number of removed queued tasks.
     */
    [[nodiscard]] std::size_t clear()
    {
      return scheduler_.clear();
    }

    /**
     * @brief Return the next task id without submitting a task.
     *
     * @return New unique task id.
     */
    [[nodiscard]] TaskId next_task_id() noexcept
    {
      return next_task_id_.fetch_add(1, std::memory_order_relaxed);
    }

  private:
    /**
     * @brief Build a ThreadPoolConfig from a worker count.
     *
     * @param threads Requested worker count.
     * @return Thread pool configuration.
     */
    [[nodiscard]] static ThreadPoolConfig make_config_from_thread_count(
        std::size_t threads)
    {
      ThreadPoolConfig config;
      config.thread_count = threads;
      config.max_thread_count = threads;
      return config.normalized();
    }

    /**
     * @brief Convert a ThreadPoolConfig into a SchedulerConfig.
     *
     * @param config Thread pool configuration.
     * @return Scheduler configuration.
     */
    [[nodiscard]] static SchedulerConfig make_scheduler_config(
        const ThreadPoolConfig &config)
    {
      SchedulerConfig schedulerConfig;
      schedulerConfig.worker_count = config.thread_count;
      schedulerConfig.max_queue_size_per_worker = config.max_queue_size;
      schedulerConfig.scheduling_policy = default_scheduling_policy();
      schedulerConfig.rejection_policy = default_rejection_policy();
      schedulerConfig.drain_on_stop = config.drain_on_shutdown;
      schedulerConfig.worker_name_prefix = "vix-tp";
      return schedulerConfig.normalized();
    }

    /**
     * @brief Merge task options with pool defaults.
     *
     * @param options Task-specific options.
     * @return Merged options.
     */
    [[nodiscard]] TaskOptions merge_options(TaskOptions options) const noexcept
    {
      if (!options.has_timeout() && config_.default_timeout.count() > 0)
      {
        options.timeout = Timeout{config_.default_timeout};
      }

      return options;
    }

    /**
     * @brief Return the next task sequence number.
     *
     * @return Monotonic sequence number.
     */
    [[nodiscard]] std::uint64_t next_sequence() noexcept
    {
      return next_sequence_.fetch_add(1, std::memory_order_relaxed);
    }

  private:
    /**
     * @brief Normalized pool configuration.
     */
    ThreadPoolConfig config_;

    /**
     * @brief Scheduler owning worker threads.
     */
    Scheduler scheduler_;

    /**
     * @brief Task id generator.
     */
    std::atomic<TaskId> next_task_id_;

    /**
     * @brief Stable queue sequence generator.
     */
    std::atomic<std::uint64_t> next_sequence_;

    /**
     * @brief Whether this pool accepts ordinary work.
     */
    std::atomic<bool> running_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_THREAD_POOL_HPP
