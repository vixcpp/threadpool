/**
 *
 * @file shutdown_bench.cpp
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
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include <vix/threadpool/all.hpp>

namespace
{
  using clock_type = std::chrono::steady_clock;

  struct BenchResult
  {
    std::string name;
    std::size_t iterations;
    std::size_t worker_count;
    std::size_t tasks_per_iteration;
    std::chrono::nanoseconds elapsed;
    double iterations_per_second;
    double average_shutdown_ms;
    std::uint64_t executed_tasks;
  };

  double to_seconds(std::chrono::nanoseconds value)
  {
    return std::chrono::duration<double>(value).count();
  }

  BenchResult run_shutdown_bench(
      std::size_t workerCount,
      std::size_t iterations,
      std::size_t tasksPerIteration,
      bool drainOnShutdown)
  {
    std::atomic<std::uint64_t> executed{0};

    const auto start = clock_type::now();

    for (std::size_t iteration = 0; iteration < iterations; ++iteration)
    {
      vix::threadpool::ThreadPoolConfig config;
      config.thread_count = workerCount;
      config.max_thread_count = workerCount;
      config.max_queue_size = 0;
      config.drain_on_shutdown = drainOnShutdown;

      vix::threadpool::ThreadPool pool(config);

      for (std::size_t task = 0; task < tasksPerIteration; ++task)
      {
        (void)pool.post(
            [&executed]()
            {
              executed.fetch_add(1, std::memory_order_relaxed);
            });
      }

      pool.shutdown();
    }

    const auto end = clock_type::now();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const double seconds = to_seconds(elapsed);
    const double throughput =
        seconds > 0.0
            ? static_cast<double>(iterations) / seconds
            : 0.0;

    const double averageShutdownMs =
        iterations > 0
            ? std::chrono::duration<double, std::milli>(elapsed).count() /
                  static_cast<double>(iterations)
            : 0.0;

    return BenchResult{
        drainOnShutdown ? "shutdown_drain" : "shutdown_no_drain",
        iterations,
        workerCount,
        tasksPerIteration,
        elapsed,
        throughput,
        averageShutdownMs,
        executed.load(std::memory_order_relaxed)};
  }

  void print_result(const BenchResult &result)
  {
    const double milliseconds =
        std::chrono::duration<double, std::milli>(result.elapsed).count();

    std::cout << result.name << '\n';
    std::cout << "  iterations: " << result.iterations << '\n';
    std::cout << "  workers: " << result.worker_count << '\n';
    std::cout << "  tasks_per_iteration: " << result.tasks_per_iteration << '\n';
    std::cout << "  executed_tasks: " << result.executed_tasks << '\n';
    std::cout << "  elapsed_ms: " << milliseconds << '\n';
    std::cout << "  iterations_per_second: "
              << static_cast<std::uint64_t>(result.iterations_per_second)
              << '\n';
    std::cout << "  average_shutdown_ms: "
              << result.average_shutdown_ms
              << '\n';
  }
}

int main()
{
  const std::size_t workerCount = 4;
  const std::size_t iterations = 1000;
  const std::size_t tasksPerIteration = 32;

  std::cout << "Vix threadpool shutdown benchmark\n";
  std::cout << "workers: " << workerCount << '\n';
  std::cout << "iterations: " << iterations << '\n';
  std::cout << "tasks_per_iteration: " << tasksPerIteration << '\n';
  std::cout << '\n';

  const BenchResult drainResult =
      run_shutdown_bench(
          workerCount,
          iterations,
          tasksPerIteration,
          true);

  const BenchResult noDrainResult =
      run_shutdown_bench(
          workerCount,
          iterations,
          tasksPerIteration,
          false);

  print_result(drainResult);
  std::cout << '\n';

  print_result(noDrainResult);

  return 0;
}
