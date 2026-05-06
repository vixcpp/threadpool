/**
 *
 * @file queue_contention_bench.cpp
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
#include <vector>

#include <vix/threadpool/all.hpp>

namespace
{
  using clock_type = std::chrono::steady_clock;

  struct BenchResult
  {
    std::string name;
    std::size_t producers;
    std::size_t tasks_per_producer;
    std::size_t submitted_tasks;
    std::size_t executed_tasks;
    std::chrono::nanoseconds elapsed;
    double submissions_per_second;
  };

  double to_seconds(std::chrono::nanoseconds value)
  {
    return std::chrono::duration<double>(value).count();
  }

  BenchResult run_queue_contention_bench(
      std::size_t workerCount,
      std::size_t producerCount,
      std::size_t tasksPerProducer)
  {
    vix::threadpool::ThreadPool pool(workerCount);

    std::atomic<std::size_t> submitted{0};
    std::atomic<std::size_t> executed{0};

    std::vector<std::thread> producers;
    producers.reserve(producerCount);

    const auto start = clock_type::now();

    for (std::size_t producer = 0; producer < producerCount; ++producer)
    {
      producers.emplace_back(
          [&pool, &submitted, &executed, tasksPerProducer]()
          {
            for (std::size_t i = 0; i < tasksPerProducer; ++i)
            {
              const bool accepted =
                  pool.post(
                      [&executed]()
                      {
                        executed.fetch_add(1, std::memory_order_relaxed);
                      });

              if (accepted)
              {
                submitted.fetch_add(1, std::memory_order_relaxed);
              }
            }
          });
    }

    for (auto &producer : producers)
    {
      if (producer.joinable())
      {
        producer.join();
      }
    }

    pool.wait_idle();

    const auto end = clock_type::now();

    pool.shutdown();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const std::size_t submittedCount =
        submitted.load(std::memory_order_relaxed);

    const double seconds = to_seconds(elapsed);
    const double throughput =
        seconds > 0.0
            ? static_cast<double>(submittedCount) / seconds
            : 0.0;

    return BenchResult{
        "queue_contention",
        producerCount,
        tasksPerProducer,
        submittedCount,
        executed.load(std::memory_order_relaxed),
        elapsed,
        throughput};
  }

  void print_result(const BenchResult &result)
  {
    const double milliseconds =
        std::chrono::duration<double, std::milli>(result.elapsed).count();

    std::cout << result.name << '\n';
    std::cout << "  producers: " << result.producers << '\n';
    std::cout << "  tasks_per_producer: " << result.tasks_per_producer << '\n';
    std::cout << "  submitted_tasks: " << result.submitted_tasks << '\n';
    std::cout << "  executed_tasks: " << result.executed_tasks << '\n';
    std::cout << "  elapsed_ms: " << milliseconds << '\n';
    std::cout << "  submissions_per_second: "
              << static_cast<std::uint64_t>(result.submissions_per_second)
              << '\n';
  }
}

int main()
{
  const std::size_t workerCount = 4;
  const std::size_t tasksPerProducer = 25000;

  std::cout << "Vix threadpool queue contention benchmark\n";
  std::cout << "workers: " << workerCount << '\n';
  std::cout << "tasks_per_producer: " << tasksPerProducer << '\n';
  std::cout << '\n';

  const BenchResult lowContention =
      run_queue_contention_bench(
          workerCount,
          2,
          tasksPerProducer);

  const BenchResult mediumContention =
      run_queue_contention_bench(
          workerCount,
          4,
          tasksPerProducer);

  const BenchResult highContention =
      run_queue_contention_bench(
          workerCount,
          8,
          tasksPerProducer);

  print_result(lowContention);
  std::cout << '\n';

  print_result(mediumContention);
  std::cout << '\n';

  print_result(highContention);

  return 0;
}
