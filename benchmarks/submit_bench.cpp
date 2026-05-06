/**
 *
 * @file submit_bench.cpp
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

#include <vix/threadpool/threadpool.hpp>

namespace
{
  using clock_type = std::chrono::steady_clock;

  struct BenchResult
  {
    std::string name;
    std::size_t tasks;
    std::chrono::nanoseconds elapsed;
    double tasks_per_second;
  };

  double to_seconds(std::chrono::nanoseconds value)
  {
    return std::chrono::duration<double>(value).count();
  }

  BenchResult run_post_bench(
      std::size_t threadCount,
      std::size_t taskCount)
  {
    vix::threadpool::ThreadPool pool(threadCount);

    std::atomic<std::size_t> counter{0};

    const auto start = clock_type::now();

    for (std::size_t i = 0; i < taskCount; ++i)
    {
      const bool accepted =
          pool.post(
              [&counter]()
              {
                counter.fetch_add(1, std::memory_order_relaxed);
              });

      if (!accepted)
      {
        break;
      }
    }

    pool.wait_idle();

    const auto end = clock_type::now();

    pool.shutdown();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const double seconds = to_seconds(elapsed);
    const double throughput =
        seconds > 0.0
            ? static_cast<double>(counter.load(std::memory_order_relaxed)) / seconds
            : 0.0;

    return BenchResult{
        "post",
        counter.load(std::memory_order_relaxed),
        elapsed,
        throughput};
  }

  BenchResult run_submit_bench(
      std::size_t threadCount,
      std::size_t taskCount)
  {
    vix::threadpool::ThreadPool pool(threadCount);

    std::vector<vix::threadpool::Future<std::size_t>> futures;
    futures.reserve(taskCount);

    const auto start = clock_type::now();

    for (std::size_t i = 0; i < taskCount; ++i)
    {
      futures.push_back(
          pool.submit(
              [i]()
              {
                return i;
              }));
    }

    std::size_t completed = 0;

    for (auto &future : futures)
    {
      (void)future.get();
      ++completed;
    }

    const auto end = clock_type::now();

    pool.shutdown();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const double seconds = to_seconds(elapsed);
    const double throughput =
        seconds > 0.0
            ? static_cast<double>(completed) / seconds
            : 0.0;

    return BenchResult{
        "submit_future",
        completed,
        elapsed,
        throughput};
  }

  void print_result(const BenchResult &result)
  {
    const double milliseconds =
        std::chrono::duration<double, std::milli>(result.elapsed).count();

    std::cout << result.name << '\n';
    std::cout << "  tasks: " << result.tasks << '\n';
    std::cout << "  elapsed_ms: " << milliseconds << '\n';
    std::cout << "  tasks_per_second: "
              << static_cast<std::uint64_t>(result.tasks_per_second)
              << '\n';
  }
}

int main()
{
  const std::size_t threadCount = 4;
  const std::size_t taskCount = 100000;

  std::cout << "Vix threadpool submit benchmark\n";
  std::cout << "threads: " << threadCount << '\n';
  std::cout << "tasks: " << taskCount << '\n';
  std::cout << '\n';

  const BenchResult postResult =
      run_post_bench(threadCount, taskCount);

  const BenchResult submitResult =
      run_submit_bench(threadCount, taskCount);

  print_result(postResult);
  std::cout << '\n';
  print_result(submitResult);

  return 0;
}
