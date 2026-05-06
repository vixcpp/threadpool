/**
 *
 * @file parallel_for_bench.cpp
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
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <vix/threadpool/threadpool.hpp>

namespace
{
  using clock_type = std::chrono::steady_clock;

  struct BenchResult
  {
    std::string name;
    std::size_t items;
    std::size_t chunk_size;
    std::chrono::nanoseconds elapsed;
    double items_per_second;
    std::uint64_t checksum;
  };

  double to_seconds(std::chrono::nanoseconds value)
  {
    return std::chrono::duration<double>(value).count();
  }

  std::uint64_t checksum_values(const std::vector<std::uint64_t> &values)
  {
    return std::accumulate(
        values.begin(),
        values.end(),
        std::uint64_t{0});
  }

  BenchResult run_parallel_for_bench(
      std::size_t threadCount,
      std::size_t itemCount,
      std::size_t chunkSize)
  {
    vix::threadpool::ThreadPool pool(threadCount);

    std::vector<std::uint64_t> values(itemCount, 0);

    vix::threadpool::ParallelForOptions options;
    options.chunk_size = chunkSize;

    const auto start = clock_type::now();

    vix::threadpool::parallel_for(
        pool,
        std::size_t{0},
        values.size(),
        [&values](std::size_t index)
        {
          values[index] =
              static_cast<std::uint64_t>(index) *
              static_cast<std::uint64_t>(index + 1);
        },
        options);

    const auto end = clock_type::now();

    pool.shutdown();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const double seconds = to_seconds(elapsed);
    const double throughput =
        seconds > 0.0
            ? static_cast<double>(itemCount) / seconds
            : 0.0;

    return BenchResult{
        "parallel_for",
        itemCount,
        chunkSize,
        elapsed,
        throughput,
        checksum_values(values)};
  }

  void print_result(const BenchResult &result)
  {
    const double milliseconds =
        std::chrono::duration<double, std::milli>(result.elapsed).count();

    std::cout << result.name << '\n';
    std::cout << "  items: " << result.items << '\n';
    std::cout << "  chunk_size: " << result.chunk_size << '\n';
    std::cout << "  elapsed_ms: " << milliseconds << '\n';
    std::cout << "  items_per_second: "
              << static_cast<std::uint64_t>(result.items_per_second)
              << '\n';
    std::cout << "  checksum: " << result.checksum << '\n';
  }
}

int main()
{
  const std::size_t threadCount = 4;
  const std::size_t itemCount = 1000000;

  std::cout << "Vix threadpool parallel_for benchmark\n";
  std::cout << "threads: " << threadCount << '\n';
  std::cout << "items: " << itemCount << '\n';
  std::cout << '\n';

  const BenchResult smallChunks =
      run_parallel_for_bench(threadCount, itemCount, 64);

  const BenchResult mediumChunks =
      run_parallel_for_bench(threadCount, itemCount, 1024);

  const BenchResult largeChunks =
      run_parallel_for_bench(threadCount, itemCount, 8192);

  print_result(smallChunks);
  std::cout << '\n';

  print_result(mediumChunks);
  std::cout << '\n';

  print_result(largeChunks);

  return 0;
}
