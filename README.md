# vix::threadpool

`vix::threadpool` is the Vix module for simple, explicit, and observable multithreaded execution in C++20.

It gives users a clean API for running concurrent and parallel work without exposing them to low-level thread management, worker loops, condition variables, queues, cancellation state, metrics, or shutdown details.

```cpp
#include <iostream>
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  std::cout << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

The user writes the work. Vix handles the execution.

## Features

- `ThreadPool` for worker-based task execution
- `post()` for fire-and-forget tasks
- `submit()` for result-producing tasks
- `TaskHandle` for cancellable tasks
- `Future`, `Promise`, and `SharedState`
- `TaskOptions` for priority, timeout, deadline, cancellation, and affinity
- `TaskPriority` for queue ordering
- `CancellationToken` and `CancellationSource`
- `Timeout` and `Deadline`
- `TaskGroup` for manual task coordination
- `Scope` for structured concurrency
- `PeriodicTask` for repeated scheduled work
- `Executor`, `InlineExecutor`, and `ThreadPoolExecutor`
- `parallel_for`
- `parallel_for_each`
- `parallel_map`
- `parallel_reduce`
- `parallel_pipeline`
- `Latch` and `Barrier`
- `ThreadPoolMetrics` and `ThreadPoolStats`
- CMake package support
- Examples, tests, benchmarks, and documentation

## Design goals

`vix::threadpool` is designed around five goals.

### Simple API

Users should be able to run work with:

```cpp
pool.post(fn);
```

or:

```cpp
auto future = pool.submit(fn);
auto value = future.get();
```

### Explicit behavior

Task behavior is configured with explicit types:

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));
```

### Safe lifecycle

The pool owns its workers and shuts them down safely.

```cpp
pool.wait_idle();
pool.shutdown();
```

Shutdown is idempotent.

### Observability

The pool exposes metrics and stats.

```cpp
const auto metrics = pool.metrics();

std::cout << metrics.pending_tasks << '\n';
std::cout << metrics.completed_tasks << '\n';
std::cout << metrics.rejected_tasks << '\n';
```

### Vix runtime integration

The module keeps the public API simple while preserving internal structure for Vix runtime systems, tests, schedulers, executors, and future modules.

## Installation

Clone the module:

```sh
git clone https://github.com/vixcpp/threadpool.git
cd threadpool
```

Configure and build:

```sh
vix build
```

With Ninja:

```sh
cmake -S . -B build-ninja -G Ninja
cmake --build build-ninja
```

### Requirements

- C++20
- CMake 3.20+
- Standard C++ threading support
- `pthread` on Linux-like systems

Supported compilers:

- GCC 11+
- Clang 14+
- Apple Clang with C++20 support

## CMake usage

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(vix_threadpool CONFIG REQUIRED)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE vix::threadpool)
```

### Add as a subdirectory

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/threadpool)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE vix::threadpool)
```

### Public include

Recommended include:

```cpp
#include <vix/threadpool/threadpool.hpp>
```

For smaller compile units, include only what you need:

```cpp
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/Scope.hpp>
```

## Basic usage

### Fire-and-forget task

Use `post()` when you do not need a return value.

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  pool.post(
      [&counter]()
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      });

  pool.wait_idle();

  std::cout << "counter: " << counter.load() << '\n';

  pool.shutdown();

  return 0;
}
```

### Submit a task and get a result

Use `submit()` when you need a return value.

```cpp
#include <iostream>
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  std::cout << "result: " << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

### Submit a task with priority

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);

pool.post(
    []()
    {
      do_important_work();
    },
    options);
```

Priority affects queued tasks only. It does not interrupt running tasks.

### Submit a task with timeout

```cpp
vix::threadpool::TaskOptions options;

options.set_timeout(
    vix::threadpool::Timeout::milliseconds(100));

auto future =
    pool.submit(
        []()
        {
          return 42;
        },
        options);
```

Timeouts are observational. Vix does not forcibly kill running C++ code.

### Submit a cancellable task

```cpp
auto handle =
    pool.handle(
        []()
        {
          return run_job();
        });

handle.cancel();

try
{
  auto value = handle.get();
}
catch (const std::exception &e)
{
  std::cout << "task failed: " << e.what() << '\n';
}
```

Cancellation is cooperative. A task can be skipped before execution if cancellation was requested early. Running C++ code is not forcefully interrupted.

## Parallel algorithms

### parallel_for

```cpp
std::vector<int> values(100, 0);

vix::threadpool::parallel_for(
    pool,
    std::size_t{0},
    values.size(),
    [&values](std::size_t index)
    {
      values[index] = static_cast<int>(index * index);
    });
```

### parallel_for_each

```cpp
std::vector<int> values{1, 2, 3, 4};

vix::threadpool::parallel_for_each(
    pool,
    values,
    [](int &value)
    {
      value *= 2;
    });
```

### parallel_map

```cpp
std::vector<int> values{1, 2, 3, 4};

std::vector<int> squares =
    vix::threadpool::parallel_map(
        pool,
        values,
        [](int value)
        {
          return value * value;
        });
```

### parallel_reduce

```cpp
std::vector<int> values{1, 2, 3, 4};

const int sum =
    vix::threadpool::parallel_reduce(
        pool,
        values,
        0,
        [](int current, int value)
        {
          return current + value;
        });
```

### parallel_pipeline

```cpp
vix::threadpool::parallel_pipeline(
    pool,
    []()
    {
      load_config();
    },
    []()
    {
      warm_cache();
    },
    []()
    {
      prepare_metrics();
    });
```

## Structured concurrency

Use `Scope` when several tasks must finish before the current operation exits.

```cpp
vix::threadpool::ThreadPool pool(4);

{
  vix::threadpool::Scope scope(pool);

  scope.spawn(
      []()
      {
        load_users();
      });

  scope.spawn(
      []()
      {
        load_products();
      });

  scope.wait_and_rethrow();
}

pool.shutdown();
```

`Scope` waits for all spawned tasks. `wait_and_rethrow()` waits for all tasks, then rethrows the first captured exception.

## Periodic tasks

`PeriodicTask` repeatedly submits a callback to an executor.

```cpp
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{100};
  config.run_immediately = true;

  vix::threadpool::PeriodicTask task(
      pool,
      [&ticks]()
      {
        const int current =
            ticks.fetch_add(1, std::memory_order_relaxed) + 1;

        std::cout << "tick: " << current << '\n';
      },
      config);

  task.start();

  std::this_thread::sleep_for(std::chrono::milliseconds{350});

  task.stop();
  task.join();

  pool.wait_idle();
  pool.shutdown();

  return 0;
}
```

Recommended shutdown order:

```cpp
task.stop();
task.join();

pool.wait_idle();
pool.shutdown();
```

## Configuration

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 4;
config.max_thread_count = 4;
config.max_queue_size = 1024;
config.default_priority = vix::threadpool::TaskPriority::normal;
config.allow_dynamic_growth = false;
config.drain_on_shutdown = true;
config.swallow_task_exceptions = true;
config.default_timeout = std::chrono::milliseconds{0};

vix::threadpool::ThreadPool pool(config);
```

The configuration is normalized internally.

## Metrics and stats

Use `metrics()` for a live snapshot:

```cpp
const auto metrics = pool.metrics();

std::cout << "workers: " << metrics.worker_count << '\n';
std::cout << "pending: " << metrics.pending_tasks << '\n';
std::cout << "active: " << metrics.active_tasks << '\n';
std::cout << "completed: " << metrics.completed_tasks << '\n';
std::cout << "failed: " << metrics.failed_tasks << '\n';
std::cout << "rejected: " << metrics.rejected_tasks << '\n';
```

Use `stats()` for cumulative counters:

```cpp
const auto stats = pool.stats();

std::cout << "accepted: " << stats.accepted_tasks << '\n';
std::cout << "completed: " << stats.completed_tasks << '\n';
std::cout << "failed: " << stats.failed_tasks << '\n';
std::cout << "rejected: " << stats.rejected_tasks << '\n';
```

## Build options

| Option | Description |
|--------|-------------|
| `VIX_THREADPOOL_BUILD_EXAMPLES` | Build examples |
| `VIX_THREADPOOL_BUILD_TESTS` | Build tests |
| `VIX_THREADPOOL_BUILD_BENCHMARKS` | Build benchmarks |

Developer build:

```sh
vix build \
  -DVIX_THREADPOOL_BUILD_EXAMPLES=ON \
  -DVIX_THREADPOOL_BUILD_TESTS=ON \
  -DVIX_THREADPOOL_BUILD_BENCHMARKS=ON
```

Release build:

```sh
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIX_THREADPOOL_BUILD_EXAMPLES=OFF \
  -DVIX_THREADPOOL_BUILD_TESTS=OFF \
  -DVIX_THREADPOOL_BUILD_BENCHMARKS=OFF

cmake --build build-release
```

## Presets

Configure:

```sh
cmake --preset dev
```

Build:

```sh
cmake --build --preset dev
```

Ninja developer build:

```sh
cmake --preset dev-ninja
cmake --build --preset dev-ninja
```

Release build:

```sh
cmake --preset release
cmake --build --preset release
```

## Examples

Build examples:

```sh
cmake -S . -B build -DVIX_THREADPOOL_BUILD_EXAMPLES=ON
cmake --build build
```

Available examples:

- `basic_post`
- `submit_future`
- `task_priority`
- `task_timeout`
- `task_cancellation`
- `task_group`
- `parallel_for`
- `parallel_for_each`
- `parallel_map`
- `parallel_reduce`
- `periodic_task`
- `metrics`
- `shutdown`
- `custom_config`

## Tests

Build tests:

```sh
cmake -S . -B build -DVIX_THREADPOOL_BUILD_TESTS=ON
cmake --build build
```

Run tests:

```sh
ctest --test-dir build --output-on-failure
```

## Benchmarks

Build benchmarks:

```sh
cmake -S . -B build -DVIX_THREADPOOL_BUILD_BENCHMARKS=ON
cmake --build build
```

Available benchmarks:

- `submit_bench`
- `parallel_for_bench`
- `parallel_map_bench`
- `queue_contention_bench`
- `shutdown_bench`

## Safety notes

### Cancellation is cooperative

Vix does not forcibly stop running C++ code.

For long-running tasks, pass a cancellation token and check it:

```cpp
if (token.cancelled())
{
  return;
}
```

### Timeouts are observational

Timeouts do not kill running tasks. They allow Vix to observe that a task exceeded its expected duration.

### Priority is not preemption

Priority affects queue ordering only. A high-priority task does not interrupt a task that is already running.

### Protect shared state

Tasks run concurrently. Shared mutable state must be protected.

```cpp
std::mutex mutex;
std::vector<int> values;

pool.post(
    [&]()
    {
      std::lock_guard<std::mutex> lock(mutex);
      values.push_back(42);
    });
```

Prefer indexed writes when possible:

```cpp
std::vector<int> output(input.size());

vix::threadpool::parallel_for(
    pool,
    std::size_t{0},
    input.size(),
    [&input, &output](std::size_t index)
    {
      output[index] = input[index] * input[index];
    });
```

## Best practices

Use one reusable pool:

```cpp
vix::threadpool::ThreadPool pool(4);
```

Use `post()` for background work:

```cpp
pool.post(fn);
```

Use `submit()` when results or exceptions matter:

```cpp
auto future = pool.submit(fn);
auto value = future.get();
```

Use `handle()` when cancellation is needed:

```cpp
auto handle = pool.handle(fn);
handle.cancel();
```

Use `Scope` for structured concurrency:

```cpp
vix::threadpool::Scope scope(pool);
scope.spawn(fn);
scope.wait_and_rethrow();
```

Use `parallel_reduce` instead of shared accumulation:

```cpp
auto sum =
    vix::threadpool::parallel_reduce(
        pool,
        values,
        0,
        [](int current, int value)
        {
          return current + value;
        });
```

Stop periodic tasks before shutting down the pool:

```cpp
periodic.stop();
periodic.join();

pool.wait_idle();
pool.shutdown();
```

## Documentation

Module docs are available in:

```
docs/
├── index.md
├── installation.md
├── quick-start.md
├── concepts.md
├── thread-pool.md
├── tasks.md
├── futures.md
├── task-groups.md
├── cancellation.md
├── timeouts.md
├── priorities.md
├── parallel-for.md
├── parallel-map.md
├── parallel-reduce.md
├── periodic-tasks.md
├── metrics.md
├── shutdown.md
├── best-practices.md
└── api-reference.md
```

## Repository

<https://github.com/vixcpp/threadpool>

## License

MIT License.

Copyright 2025, Gaspard Kirira.
