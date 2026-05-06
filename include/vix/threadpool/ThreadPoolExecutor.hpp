/**
 *
 * @file ThreadPoolExecutor.hpp
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
#ifndef VIX_THREADPOOL_THREAD_POOL_EXECUTOR_HPP
#define VIX_THREADPOOL_THREAD_POOL_EXECUTOR_HPP

#include <utility>

#include <vix/threadpool/Executor.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>

namespace vix::threadpool
{
  class ThreadPool;

  /**
   * @brief Executor adapter backed by a concrete ThreadPool.
   *
   * ThreadPoolExecutor provides the generic Executor interface on top of an
   * existing ThreadPool instance.
   *
   * This adapter is non-owning:
   * - the referenced ThreadPool must outlive the ThreadPoolExecutor
   * - shutdown() forwards to the referenced pool
   * - metrics() and stats() are read from the referenced pool
   *
   * The implementation is kept in the .cpp file so this header can forward
   * declare ThreadPool and avoid circular includes.
   */
  class ThreadPoolExecutor final : public Executor
  {
  public:
    /**
     * @brief Construct an invalid executor adapter.
     */
    ThreadPoolExecutor() noexcept;

    /**
     * @brief Construct an executor adapter from a ThreadPool reference.
     *
     * @param pool Referenced thread pool.
     */
    explicit ThreadPoolExecutor(ThreadPool &pool) noexcept;

    ThreadPoolExecutor(const ThreadPoolExecutor &) = delete;
    ThreadPoolExecutor &operator=(const ThreadPoolExecutor &) = delete;

    /**
     * @brief Move-construct an executor adapter.
     */
    ThreadPoolExecutor(ThreadPoolExecutor &&other) noexcept;

    /**
     * @brief Move-assign an executor adapter.
     *
     * @param other Source adapter.
     * @return Reference to this adapter.
     */
    ThreadPoolExecutor &operator=(ThreadPoolExecutor &&other) noexcept;

    /**
     * @brief Destroy the adapter.
     *
     * The referenced ThreadPool is not destroyed by this object.
     */
    ~ThreadPoolExecutor() override;

    /**
     * @brief Bind this adapter to a ThreadPool.
     *
     * @param pool Referenced thread pool.
     */
    void reset(ThreadPool &pool) noexcept;

    /**
     * @brief Clear the referenced ThreadPool.
     *
     * After this call, post() returns false and metrics() returns an empty
     * snapshot.
     */
    void reset() noexcept;

    /**
     * @brief Check whether this adapter references a ThreadPool.
     *
     * @return true if a pool is referenced.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Bool conversion.
     *
     * @return true if a pool is referenced.
     */
    [[nodiscard]] explicit operator bool() const noexcept;

    /**
     * @brief Post one fire-and-forget task to the referenced ThreadPool.
     *
     * @param task Callable to execute.
     * @param options Task submission options.
     * @return true if the task was accepted or handled successfully.
     */
    [[nodiscard]] bool post(
        Task task,
        TaskOptions options = TaskOptions{}) override;

    /**
     * @brief Request shutdown on the referenced ThreadPool.
     *
     * This function does nothing when no pool is referenced.
     */
    void shutdown() noexcept override;

    /**
     * @brief Wait until the referenced ThreadPool is idle.
     *
     * This function does nothing when no pool is referenced.
     */
    void wait_idle() override;

    /**
     * @brief Check whether the referenced ThreadPool is running.
     *
     * @return true if a pool is referenced and running.
     */
    [[nodiscard]] bool running() const noexcept override;

    /**
     * @brief Check whether the referenced ThreadPool is idle.
     *
     * @return true if no pool is referenced or the referenced pool is idle.
     */
    [[nodiscard]] bool idle() const override;

    /**
     * @brief Return metrics from the referenced ThreadPool.
     *
     * @return Metrics snapshot, or an empty snapshot when no pool is referenced.
     */
    [[nodiscard]] ThreadPoolMetrics metrics() const override;

    /**
     * @brief Return stats from the referenced ThreadPool.
     *
     * @return Stats snapshot, or an empty snapshot when no pool is referenced.
     */
    [[nodiscard]] ThreadPoolStats stats() const override;

    /**
     * @brief Return the referenced ThreadPool.
     *
     * @return ThreadPool pointer, or nullptr.
     */
    [[nodiscard]] ThreadPool *pool() noexcept;

    /**
     * @brief Return the referenced ThreadPool.
     *
     * @return ThreadPool pointer, or nullptr.
     */
    [[nodiscard]] const ThreadPool *pool() const noexcept;

  private:
    /**
     * @brief Referenced ThreadPool.
     */
    ThreadPool *pool_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_THREAD_POOL_EXECUTOR_HPP
