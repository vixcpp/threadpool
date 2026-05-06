/**
 *
 * @file WorkerState.hpp
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
#ifndef VIX_THREADPOOL_WORKER_STATE_HPP
#define VIX_THREADPOOL_WORKER_STATE_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Lifecycle state of a worker thread.
   *
   * WorkerState describes the current state of a worker owned by a thread pool.
   */
  enum class WorkerState : std::uint8_t
  {
    /**
     * @brief Worker has been created but not started.
     */
    created = 0,

    /**
     * @brief Worker is waiting for work.
     */
    idle = 1,

    /**
     * @brief Worker is currently executing a task.
     */
    running = 2,

    /**
     * @brief Worker has been asked to stop.
     */
    stopping = 3,

    /**
     * @brief Worker has stopped and will not execute more work.
     */
    stopped = 4,

    /**
     * @brief Worker failed because of an unrecoverable internal error.
     */
    failed = 5
  };

  /**
   * @brief Check whether a worker state is terminal.
   *
   * @param state Worker state to inspect.
   * @return true if the worker reached a final state, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_terminal(WorkerState state) noexcept
  {
    return state == WorkerState::stopped ||
           state == WorkerState::failed;
  }

  /**
   * @brief Check whether a worker state can execute work.
   *
   * @param state Worker state to inspect.
   * @return true if the worker may execute tasks, false otherwise.
   */
  [[nodiscard]] inline constexpr bool can_execute(WorkerState state) noexcept
  {
    return state == WorkerState::idle ||
           state == WorkerState::running;
  }

  /**
   * @brief Return a readable worker state name.
   *
   * @param state Worker state.
   * @return Static state name.
   */
  [[nodiscard]] inline constexpr const char *to_string(WorkerState state) noexcept
  {
    switch (state)
    {
    case WorkerState::created:
      return "created";
    case WorkerState::idle:
      return "idle";
    case WorkerState::running:
      return "running";
    case WorkerState::stopping:
      return "stopping";
    case WorkerState::stopped:
      return "stopped";
    case WorkerState::failed:
      return "failed";
    }

    return "unknown";
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_WORKER_STATE_HPP
