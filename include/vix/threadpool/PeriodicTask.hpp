/**
 *
 * @file PeriodicTask.hpp
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
#ifndef VIX_THREADPOOL_PERIODIC_TASK_HPP
#define VIX_THREADPOOL_PERIODIC_TASK_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

#include <vix/threadpool/Executor.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>

namespace vix::threadpool
{
  /**
   * @brief Configuration for a periodic task.
   *
   * PeriodicTaskConfig controls how often a callback is submitted to an executor
   * and whether the first execution should happen immediately.
   */
  struct PeriodicTaskConfig
  {
    /**
     * @brief Interval between task submissions.
     */
    std::chrono::milliseconds interval;

    /**
     * @brief Options used for every submitted callback.
     */
    TaskOptions options;

    /**
     * @brief Whether the first callback should be submitted immediately.
     */
    bool run_immediately;

    /**
     * @brief Whether the scheduler thread should stop when post() fails.
     */
    bool stop_on_post_failure;

    /**
     * @brief Construct default periodic task configuration.
     */
    PeriodicTaskConfig() noexcept
        : interval(std::chrono::milliseconds{1000}),
          options(),
          run_immediately(false),
          stop_on_post_failure(true)
    {
    }

    /**
     * @brief Create a periodic task configuration from an interval.
     *
     * @param value Interval between executions.
     * @return Configured periodic task configuration.
     */
    [[nodiscard]] static PeriodicTaskConfig every(
        std::chrono::milliseconds value) noexcept
    {
      PeriodicTaskConfig config;
      config.interval = normalize_interval(value);
      return config;
    }

    /**
     * @brief Normalize an interval value.
     *
     * Non-positive intervals are converted to one millisecond.
     *
     * @param value Requested interval.
     * @return Normalized interval.
     */
    [[nodiscard]] static constexpr std::chrono::milliseconds normalize_interval(
        std::chrono::milliseconds value) noexcept
    {
      return value.count() <= 0
                 ? std::chrono::milliseconds{1}
                 : value;
    }

    /**
     * @brief Return a normalized copy of this configuration.
     *
     * @return Normalized configuration.
     */
    [[nodiscard]] PeriodicTaskConfig normalized() const noexcept
    {
      PeriodicTaskConfig out = *this;
      out.interval = normalize_interval(out.interval);
      return out;
    }
  };

  /**
   * @brief Periodically submits a callback to an executor.
   *
   * PeriodicTask owns one lightweight scheduler thread. That scheduler thread
   * does not execute the user callback directly. Instead, it posts the callback
   * to the configured executor at each interval.
   *
   * This keeps the periodic scheduling logic separate from real task execution.
   */
  class PeriodicTask
  {
  public:
    /**
     * @brief Callback type submitted periodically.
     */
    using Callback = std::function<void()>;

    /**
     * @brief Construct an empty periodic task.
     */
    PeriodicTask() noexcept
        : executor_(),
          callback_(),
          config_(),
          running_(false),
          submitted_ticks_(0),
          failed_posts_(0),
          thread_()
    {
    }

    /**
     * @brief Construct a periodic task.
     *
     * The task is not started automatically. Call start() explicitly.
     *
     * @param executor Executor receiving periodic callbacks.
     * @param callback Callback submitted at each tick.
     * @param config Periodic task configuration.
     */
    PeriodicTask(
        Executor &executor,
        Callback callback,
        PeriodicTaskConfig config = PeriodicTaskConfig{})
        : executor_(executor),
          callback_(std::move(callback)),
          config_(config.normalized()),
          running_(false),
          submitted_ticks_(0),
          failed_posts_(0),
          thread_()
    {
    }

    PeriodicTask(const PeriodicTask &) = delete;
    PeriodicTask &operator=(const PeriodicTask &) = delete;

    /**
     * @brief Move-construct a periodic task.
     *
     * @param other Source periodic task.
     */
    PeriodicTask(PeriodicTask &&other) noexcept
        : executor_(other.executor_),
          callback_(std::move(other.callback_)),
          config_(other.config_),
          running_(other.running_.load(std::memory_order_acquire)),
          submitted_ticks_(other.submitted_ticks_.load(std::memory_order_relaxed)),
          failed_posts_(other.failed_posts_.load(std::memory_order_relaxed)),
          thread_(std::move(other.thread_))
    {
      other.running_.store(false, std::memory_order_release);
      other.executor_ = ExecutorRef{};
    }

    /**
     * @brief Move-assign a periodic task.
     *
     * @param other Source periodic task.
     * @return Reference to this periodic task.
     */
    PeriodicTask &operator=(PeriodicTask &&other) noexcept
    {
      if (this == &other)
      {
        return *this;
      }

      stop();
      join();

      executor_ = other.executor_;
      callback_ = std::move(other.callback_);
      config_ = other.config_;
      running_.store(other.running_.load(std::memory_order_acquire),
                     std::memory_order_release);
      submitted_ticks_.store(
          other.submitted_ticks_.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      failed_posts_.store(
          other.failed_posts_.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      thread_ = std::move(other.thread_);

      other.running_.store(false, std::memory_order_release);
      other.executor_ = ExecutorRef{};

      return *this;
    }

    /**
     * @brief Stop and join the periodic scheduler thread.
     */
    ~PeriodicTask() noexcept
    {
      stop();
      join();
    }

    /**
     * @brief Start periodic submission.
     *
     * Calling start() when already running has no effect.
     *
     * @return true if the scheduler thread was started.
     */
    [[nodiscard]] bool start()
    {
      if (!executor_.valid() || !callback_)
      {
        return false;
      }

      bool expected = false;
      if (!running_.compare_exchange_strong(
              expected,
              true,
              std::memory_order_acq_rel,
              std::memory_order_acquire))
      {
        return false;
      }

      try
      {
        thread_ = std::thread(
            [this]()
            {
              run_loop();
            });

        return true;
      }
      catch (...)
      {
        running_.store(false, std::memory_order_release);
        return false;
      }
    }

    /**
     * @brief Request periodic submission to stop.
     *
     * This operation is idempotent.
     */
    void stop() noexcept
    {
      running_.store(false, std::memory_order_release);
    }

    /**
     * @brief Join the scheduler thread if joinable.
     */
    void join() noexcept
    {
      if (!thread_.joinable())
      {
        return;
      }

      if (thread_.get_id() == std::this_thread::get_id())
      {
        thread_.detach();
        return;
      }

      try
      {
        thread_.join();
      }
      catch (...)
      {
        try
        {
          thread_.detach();
        }
        catch (...)
        {
        }
      }
    }

    /**
     * @brief Check whether periodic submission is running.
     *
     * @return true if running.
     */
    [[nodiscard]] bool running() const noexcept
    {
      return running_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check whether the scheduler thread is joinable.
     *
     * @return true if joinable.
     */
    [[nodiscard]] bool joinable() const noexcept
    {
      return thread_.joinable();
    }

    /**
     * @brief Return the periodic task configuration.
     *
     * @return Configuration.
     */
    [[nodiscard]] const PeriodicTaskConfig &config() const noexcept
    {
      return config_;
    }

    /**
     * @brief Return the number of successfully submitted ticks.
     *
     * @return Submitted tick count.
     */
    [[nodiscard]] std::uint64_t submitted_ticks() const noexcept
    {
      return submitted_ticks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return the number of failed post attempts.
     *
     * @return Failed post count.
     */
    [[nodiscard]] std::uint64_t failed_posts() const noexcept
    {
      return failed_posts_.load(std::memory_order_relaxed);
    }

  private:
    /**
     * @brief Main scheduler loop.
     */
    void run_loop()
    {
      if (config_.run_immediately)
      {
        submit_tick();
      }

      auto next = std::chrono::steady_clock::now() + config_.interval;

      while (running_.load(std::memory_order_acquire))
      {
        std::this_thread::sleep_until(next);

        if (!running_.load(std::memory_order_acquire))
        {
          break;
        }

        submit_tick();
        next += config_.interval;
      }
    }

    /**
     * @brief Submit one periodic callback to the executor.
     */
    void submit_tick()
    {
      if (!executor_.valid())
      {
        failed_posts_.fetch_add(1, std::memory_order_relaxed);
        running_.store(false, std::memory_order_release);
        return;
      }

      const bool ok = executor_.post(callback_, config_.options);
      if (ok)
      {
        submitted_ticks_.fetch_add(1, std::memory_order_relaxed);
        return;
      }

      failed_posts_.fetch_add(1, std::memory_order_relaxed);

      if (config_.stop_on_post_failure)
      {
        running_.store(false, std::memory_order_release);
      }
    }

  private:
    /**
     * @brief Executor receiving periodic callbacks.
     */
    ExecutorRef executor_;

    /**
     * @brief Callback submitted on each tick.
     */
    Callback callback_;

    /**
     * @brief Periodic task configuration.
     */
    PeriodicTaskConfig config_;

    /**
     * @brief Whether periodic submission is running.
     */
    std::atomic<bool> running_;

    /**
     * @brief Number of successfully submitted callbacks.
     */
    std::atomic<std::uint64_t> submitted_ticks_;

    /**
     * @brief Number of failed post attempts.
     */
    std::atomic<std::uint64_t> failed_posts_;

    /**
     * @brief Scheduler thread.
     */
    std::thread thread_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PERIODIC_TASK_HPP
