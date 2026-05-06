/**
 *
 * @file this_worker.hpp
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
#ifndef VIX_THREADPOOL_THIS_WORKER_HPP
#define VIX_THREADPOOL_THIS_WORKER_HPP

#include <cstddef>

#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/WorkerId.hpp>

namespace vix::threadpool::this_worker
{
  /**
   * @brief Thread-local context for the currently running worker.
   *
   * This context is set by WorkerThread before executing tasks and cleared when
   * the worker thread exits.
   */
  struct Context
  {
    /**
     * @brief Current worker identifier.
     */
    WorkerId worker_id;

    /**
     * @brief Current task identifier.
     */
    TaskId task_id;

    /**
     * @brief Worker index inside the owning pool.
     */
    std::size_t worker_index;

    /**
     * @brief Whether the current thread is a threadpool worker.
     */
    bool inside_worker;

    /**
     * @brief Construct an empty context.
     */
    constexpr Context() noexcept
        : worker_id(invalid_worker_id),
          task_id(invalid_task_id),
          worker_index(0),
          inside_worker(false)
    {
    }
  };

  /**
   * @brief Thread-local worker context.
   */
  inline thread_local Context context{};

  /**
   * @brief Check whether the current thread is a threadpool worker.
   *
   * @return true if running inside a Vix threadpool worker.
   */
  [[nodiscard]] inline bool inside() noexcept
  {
    return context.inside_worker;
  }

  /**
   * @brief Return the current worker id.
   *
   * @return Worker id, or invalid_worker_id outside a worker.
   */
  [[nodiscard]] inline WorkerId id() noexcept
  {
    return context.worker_id;
  }

  /**
   * @brief Return the current worker index.
   *
   * @return Worker index, or 0 outside a worker.
   */
  [[nodiscard]] inline std::size_t index() noexcept
  {
    return context.worker_index;
  }

  /**
   * @brief Return the currently running task id.
   *
   * @return Task id, or invalid_task_id if no task is running.
   */
  [[nodiscard]] inline TaskId task_id() noexcept
  {
    return context.task_id;
  }

  /**
   * @brief Set the current worker context.
   *
   * This function is intended for WorkerThread internals.
   *
   * @param workerId Worker identifier.
   * @param workerIndex Worker index.
   */
  inline void set(WorkerId workerId, std::size_t workerIndex) noexcept
  {
    context.worker_id = workerId;
    context.worker_index = workerIndex;
    context.task_id = invalid_task_id;
    context.inside_worker = true;
  }

  /**
   * @brief Set the currently running task id.
   *
   * This function is intended for WorkerThread internals.
   *
   * @param taskId Task identifier.
   */
  inline void set_task(TaskId taskId) noexcept
  {
    context.task_id = taskId;
  }

  /**
   * @brief Clear the currently running task id.
   *
   * This function is intended for WorkerThread internals.
   */
  inline void clear_task() noexcept
  {
    context.task_id = invalid_task_id;
  }

  /**
   * @brief Clear the current worker context.
   *
   * This function is intended for WorkerThread internals.
   */
  inline void clear() noexcept
  {
    context = Context{};
  }

} // namespace vix::threadpool::this_worker

#endif // VIX_THREADPOOL_THIS_WORKER_HPP
