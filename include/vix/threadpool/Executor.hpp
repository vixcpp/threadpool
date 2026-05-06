/**
 *
 * @file Executor.hpp
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
#ifndef VIX_THREADPOOL_EXECUTOR_HPP
#define VIX_THREADPOOL_EXECUTOR_HPP

#include <cstdint>
#include <functional>

#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>

namespace vix::threadpool
{
  /**
   * @brief Minimal executor interface for fire-and-forget work.
   *
   * Executor is the stable runtime abstraction used by higher-level Vix modules
   * when they only need to submit void callbacks without depending directly on
   * a concrete ThreadPool implementation.
   *
   * This interface intentionally stays small:
   * - post work
   * - stop execution
   * - wait for idle state
   * - expose metrics and stats
   *
   * Result-producing APIs are kept on concrete executors because templated
   * submit() cannot be virtual in C++.
   */
  class Executor
  {
  public:
    /**
     * @brief Callable type accepted by the executor.
     */
    using Task = std::function<void()>;

    /**
     * @brief Virtual destructor.
     */
    virtual ~Executor() = default;

    /**
     * @brief Post one fire-and-forget task.
     *
     * @param task Callable to execute.
     * @param options Task submission options.
     * @return true if the task was accepted or handled successfully.
     */
    [[nodiscard]] virtual bool post(
        Task task,
        TaskOptions options = TaskOptions{}) = 0;

    /**
     * @brief Request executor shutdown.
     *
     * Shutdown should be safe and idempotent.
     */
    virtual void shutdown() noexcept = 0;

    /**
     * @brief Wait until the executor has no pending or active work.
     */
    virtual void wait_idle() = 0;

    /**
     * @brief Check whether the executor is currently running.
     *
     * @return true if the executor accepts ordinary work.
     */
    [[nodiscard]] virtual bool running() const noexcept = 0;

    /**
     * @brief Check whether the executor has no pending or active work.
     *
     * @return true if the executor is idle.
     */
    [[nodiscard]] virtual bool idle() const = 0;

    /**
     * @brief Return executor metrics.
     *
     * @return Threadpool metrics snapshot.
     */
    [[nodiscard]] virtual ThreadPoolMetrics metrics() const = 0;

    /**
     * @brief Return executor cumulative stats.
     *
     * @return Threadpool stats snapshot.
     */
    [[nodiscard]] virtual ThreadPoolStats stats() const = 0;
  };

  /**
   * @brief Non-owning executor reference wrapper.
   *
   * ExecutorRef is useful when an API wants to store or pass an executor without
   * taking ownership of it.
   */
  class ExecutorRef
  {
  public:
    /**
     * @brief Construct an empty executor reference.
     */
    ExecutorRef() noexcept
        : executor_(nullptr)
    {
    }

    /**
     * @brief Construct an executor reference.
     *
     * @param executor Referenced executor.
     */
    explicit ExecutorRef(Executor &executor) noexcept
        : executor_(&executor)
    {
    }

    /**
     * @brief Check whether this reference points to an executor.
     *
     * @return true if a valid executor is referenced.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return executor_ != nullptr;
    }

    /**
     * @brief Bool conversion.
     *
     * @return true if a valid executor is referenced.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return valid();
    }

    /**
     * @brief Return the referenced executor.
     *
     * @return Executor reference.
     */
    [[nodiscard]] Executor &get() const noexcept
    {
      return *executor_;
    }

    /**
     * @brief Post one fire-and-forget task to the referenced executor.
     *
     * @param task Callable to execute.
     * @param options Task submission options.
     * @return true if the task was accepted or handled successfully.
     */
    [[nodiscard]] bool post(
        Executor::Task task,
        TaskOptions options = TaskOptions{}) const
    {
      return executor_ != nullptr &&
             executor_->post(std::move(task), std::move(options));
    }

    /**
     * @brief Request shutdown on the referenced executor if present.
     */
    void shutdown() const noexcept
    {
      if (executor_ != nullptr)
      {
        executor_->shutdown();
      }
    }

    /**
     * @brief Wait until the referenced executor is idle.
     */
    void wait_idle() const
    {
      if (executor_ != nullptr)
      {
        executor_->wait_idle();
      }
    }

    /**
     * @brief Check whether the referenced executor is running.
     *
     * @return true if a referenced executor is running.
     */
    [[nodiscard]] bool running() const noexcept
    {
      return executor_ != nullptr && executor_->running();
    }

    /**
     * @brief Check whether the referenced executor is idle.
     *
     * @return true if no executor is referenced or the executor is idle.
     */
    [[nodiscard]] bool idle() const
    {
      return executor_ == nullptr || executor_->idle();
    }

    /**
     * @brief Return referenced executor metrics.
     *
     * @return Metrics snapshot, or an empty snapshot when no executor is referenced.
     */
    [[nodiscard]] ThreadPoolMetrics metrics() const
    {
      return executor_ != nullptr ? executor_->metrics() : ThreadPoolMetrics{};
    }

    /**
     * @brief Return referenced executor stats.
     *
     * @return Stats snapshot, or an empty snapshot when no executor is referenced.
     */
    [[nodiscard]] ThreadPoolStats stats() const
    {
      return executor_ != nullptr ? executor_->stats() : ThreadPoolStats{};
    }

  private:
    /**
     * @brief Referenced executor.
     */
    Executor *executor_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_EXECUTOR_HPP
