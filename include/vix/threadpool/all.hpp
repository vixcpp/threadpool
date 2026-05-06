/**
 *
 * @file threadpool.hpp
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
#ifndef VIX_THREADPOOL_THREADPOOL_HPP
#define VIX_THREADPOOL_THREADPOOL_HPP

/**
 * @brief Public umbrella header for the Vix threadpool module.
 *
 * Include this file when you want access to the complete public threadpool API:
 *
 * @code
 * #include <vix/threadpool/all.hpp>
 * @endcode
 *
 * This header exposes:
 * - task identifiers and task options
 * - cancellation, timeout, and deadline helpers
 * - futures, promises, task handles, and task groups
 * - worker and scheduler types
 * - executor abstractions
 * - the main ThreadPool API
 * - high-level parallel algorithms
 * - structured concurrency helpers
 */

#include <vix/threadpool/version.hpp>

#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/WorkerId.hpp>

#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/WorkerState.hpp>

#include <vix/threadpool/ThreadPoolError.hpp>
#include <vix/threadpool/ThreadPoolConfig.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>

#include <vix/threadpool/CancellationToken.hpp>
#include <vix/threadpool/CancellationSource.hpp>
#include <vix/threadpool/Timeout.hpp>
#include <vix/threadpool/Deadline.hpp>

#include <vix/threadpool/SharedState.hpp>
#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/Promise.hpp>

#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskCmp.hpp>
#include <vix/threadpool/TaskGuard.hpp>
#include <vix/threadpool/TaskHandle.hpp>
#include <vix/threadpool/TaskQueue.hpp>
#include <vix/threadpool/TaskGroup.hpp>

#include <vix/threadpool/this_worker.hpp>
#include <vix/threadpool/WorkerThread.hpp>
#include <vix/threadpool/Worker.hpp>

#include <vix/threadpool/SchedulingPolicy.hpp>
#include <vix/threadpool/QueuePolicy.hpp>
#include <vix/threadpool/RejectionPolicy.hpp>
#include <vix/threadpool/Scheduler.hpp>

#include <vix/threadpool/ExecutorTraits.hpp>
#include <vix/threadpool/Executor.hpp>
#include <vix/threadpool/InlineExecutor.hpp>
#include <vix/threadpool/ThreadPoolExecutor.hpp>

#include <vix/threadpool/PeriodicTask.hpp>
#include <vix/threadpool/ThreadPool.hpp>

#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/ParallelForEach.hpp>
#include <vix/threadpool/ParallelMap.hpp>
#include <vix/threadpool/ParallelReduce.hpp>
#include <vix/threadpool/ParallelPipeline.hpp>
#include <vix/threadpool/Parallel.hpp>

#include <vix/threadpool/Scope.hpp>
#include <vix/threadpool/Latch.hpp>
#include <vix/threadpool/Barrier.hpp>

namespace vix::threadpool
{
  /**
   * @brief Return the threadpool module version string.
   *
   * @return Semantic version string.
   */
  [[nodiscard]] inline constexpr const char *module_version() noexcept
  {
    return version;
  }

  /**
   * @brief Return whether the threadpool module is available.
   *
   * This helper is useful for compile-time checks in higher-level modules.
   *
   * @return true.
   */
  [[nodiscard]] inline constexpr bool available() noexcept
  {
    return true;
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_THREADPOOL_HPP
