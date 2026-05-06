# Changelog

All notable changes to `vix::threadpool` will be documented in this file.

This project follows semantic versioning.

## v0.1.0

### Added
- Added the initial `vix::threadpool` module.
- Added `ThreadPool` as the main public API for multithreaded task execution.
- Added fire-and-forget task submission with `post()`.
- Added result-producing task submission with `submit()`.
- Added cancellable task handles with `TaskHandle`.
- Added `Future`, `Promise`, and `SharedState` for task result propagation.
- Added support for `void` and value-returning tasks.
- Added task lifecycle tracking with `TaskStatus`.
- Added task result tracking with `TaskResult`.
- Added task priorities with `TaskPriority`.
- Added task options with `TaskOptions`.
- Added task identifiers with `TaskId`.
- Added worker identifiers with `WorkerId`.
- Added cancellation support with `CancellationToken` and `CancellationSource`.
- Added timeout support with `Timeout`.
- Added deadline support with `Deadline`.
- Added threadpool error codes with `ThreadPoolErrc`.
- Added threadpool configuration with `ThreadPoolConfig`.
- Added live metrics with `ThreadPoolMetrics`.
- Added cumulative stats with `ThreadPoolStats`.
- Added internal task queue support with `TaskQueue`.
- Added task ordering with `TaskCmp`.
- Added task activity tracking with `TaskGuard`.
- Added worker lifecycle types with `Worker`, `WorkerThread`, and `WorkerState`.
- Added worker-local helpers in `this_worker.hpp`.
- Added scheduling policies with `SchedulingPolicy`.
- Added queue policies with `QueuePolicy`.
- Added rejection policies with `RejectionPolicy`.
- Added `Scheduler` for distributing work across workers.
- Added executor abstraction with `Executor`.
- Added `ExecutorRef` for non-owning executor access.
- Added `InlineExecutor` for deterministic inline execution.
- Added `ThreadPoolExecutor` for adapting `ThreadPool` to the executor interface.
- Added `TaskGroup` for manual task coordination.
- Added `Scope` for structured concurrency.
- Added `PeriodicTask` for repeated scheduled submissions.
- Added `Latch` as a one-shot synchronization primitive.
- Added `Barrier` as a reusable synchronization primitive.
- Added `parallel_for` for index-based parallel loops.
- Added `parallel_for_each` for container and iterator-range processing.
- Added `parallel_map` for ordered parallel transformations.
- Added `parallel_reduce` for parallel aggregation.
- Added `parallel_pipeline` for running independent stages concurrently.
- Added `parallel` namespace helpers.
- Added detail utilities:
  - `AtomicCounter`
  - `FunctionTraits`
  - `Invoke`
  - `LockUtils`
  - `ScopeExit`
  - `MoveOnlyFunction`
  - `ThreadName`
  - `WaitStrategy`
- Added module umbrella header `<vix/threadpool/threadpool.hpp>`.
- Added module version header `<vix/threadpool/version.hpp>`.
- Added standalone CMake support with `vix::threadpool`.
- Added standalone package export support with `vix_threadpoolTargets`.
- Added Vix umbrella build export support through `VixTargets`.
- Added CMake presets for development and release builds.
- Added examples for:
  - basic task posting
  - futures
  - priorities
  - timeouts
  - cancellation
  - task groups
  - parallel loops
  - parallel map
  - parallel reduce
  - periodic tasks
  - metrics
  - shutdown
  - custom configuration
- Added unit tests for configuration, metrics, queues, priorities, cancellation, timeouts, task groups, workers, executors, thread pool behavior, parallel algorithms, periodic tasks, scopes, shutdown, and stress scenarios.
- Added benchmarks for submission, parallel loops, parallel map, queue contention, and shutdown.
- Added documentation for installation, quick start, concepts, tasks, futures, task groups, cancellation, timeouts, priorities, parallel algorithms, periodic tasks, metrics, shutdown, best practices, and API reference.

### Notes
- Cancellation is cooperative.
- Timeouts are observational.
- Priorities affect queued tasks only and do not preempt running work.
- Shutdown is idempotent.
- The public API is designed to stay simple while keeping scheduling, queues, metrics, and worker complexity inside Vix.
