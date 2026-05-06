/**
 *
 * @file StressTest.cpp
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
#include <numeric>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/ParallelMap.hpp>
#include <vix/threadpool/ParallelReduce.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/ThreadPoolConfig.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>

namespace
{
  bool wait_until_true(std::function<bool()> predicate)
  {
    for (int i = 0; i < 200; ++i)
    {
      if (predicate())
      {
        return true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }

    return false;
  }
}

TEST(StressTest, ExecutesManyPostedTasks)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr int taskCount = 1000;
  std::atomic<int> counter{0};

  for (int i = 0; i < taskCount; ++i)
  {
    const bool accepted =
        pool.post(
            [&counter]()
            {
              counter.fetch_add(1, std::memory_order_relaxed);
            });

    EXPECT_TRUE(accepted);
  }

  pool.wait_idle();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), taskCount);

  const vix::threadpool::ThreadPoolMetrics metrics = pool.metrics();

  EXPECT_EQ(metrics.pending_tasks, std::size_t{0});
  EXPECT_GE(metrics.completed_tasks, std::uint64_t{taskCount});
  EXPECT_TRUE(metrics.idle());

  pool.shutdown();
}

TEST(StressTest, ExecutesManyFutureTasks)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr int taskCount = 500;

  std::vector<vix::threadpool::Future<int>> futures;
  futures.reserve(taskCount);

  for (int i = 0; i < taskCount; ++i)
  {
    futures.push_back(
        pool.submit(
            [i]()
            {
              return i;
            }));
  }

  long long sum = 0;

  for (auto &future : futures)
  {
    sum += future.get();
  }

  const long long expected =
      static_cast<long long>(taskCount - 1) *
      static_cast<long long>(taskCount) / 2;

  EXPECT_EQ(sum, expected);

  pool.shutdown();
}

TEST(StressTest, HandlesMixedPriorityLoad)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr int taskCount = 300;
  std::atomic<int> counter{0};

  for (int i = 0; i < taskCount; ++i)
  {
    vix::threadpool::TaskOptions options;

    if (i % 3 == 0)
    {
      options.set_priority(vix::threadpool::TaskPriority::high);
    }
    else if (i % 3 == 1)
    {
      options.set_priority(vix::threadpool::TaskPriority::normal);
    }
    else
    {
      options.set_priority(vix::threadpool::TaskPriority::low);
    }

    EXPECT_TRUE(
        pool.post(
            [&counter]()
            {
              counter.fetch_add(1, std::memory_order_relaxed);
            },
            options));
  }

  pool.wait_idle();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), taskCount);

  pool.shutdown();
}

TEST(StressTest, ParallelForLargeRange)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr std::size_t count = 2048;

  std::vector<int> values(count, 0);

  vix::threadpool::parallel_for(
      pool,
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = static_cast<int>(index + 1);
      },
      vix::threadpool::ParallelForOptions::with_chunk_size(32));

  for (std::size_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(values[i], static_cast<int>(i + 1));
  }

  pool.shutdown();
}

TEST(StressTest, ParallelMapLargeRange)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr std::size_t count = 1024;

  std::vector<int> values(count);

  std::iota(values.begin(), values.end(), 1);

  const std::vector<int> output =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            return value * 2;
          },
          vix::threadpool::ParallelMapOptions::with_chunk_size(32));

  ASSERT_EQ(output.size(), values.size());

  for (std::size_t i = 0; i < output.size(); ++i)
  {
    EXPECT_EQ(output[i], values[i] * 2);
  }

  pool.shutdown();
}

TEST(StressTest, ParallelReduceLargeRange)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr int count = 1000;

  std::vector<int> values(count);

  std::iota(values.begin(), values.end(), 1);

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            return current + value;
          },
          vix::threadpool::ParallelReduceOptions::with_chunk_size(25));

  EXPECT_EQ(result, 500500);

  pool.shutdown();
}

TEST(StressTest, RepeatedCreateAndShutdownIsSafe)
{
  for (int round = 0; round < 20; ++round)
  {
    vix::threadpool::ThreadPool pool(2);

    std::atomic<int> counter{0};

    for (int i = 0; i < 20; ++i)
    {
      pool.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });
    }

    pool.wait_idle();

    EXPECT_EQ(counter.load(std::memory_order_relaxed), 20);

    pool.shutdown();

    EXPECT_FALSE(pool.running());
  }
}

TEST(StressTest, BoundedQueueRejectsSomeTasksUnderPressure)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 1;
  config.max_thread_count = 1;
  config.max_queue_size = 4;
  config.drain_on_shutdown = true;

  vix::threadpool::ThreadPool pool(config);

  std::atomic<int> completed{0};
  int accepted = 0;
  int rejected = 0;

  for (int i = 0; i < 50; ++i)
  {
    const bool ok =
        pool.post(
            [&completed]()
            {
              std::this_thread::sleep_for(std::chrono::milliseconds{2});
              completed.fetch_add(1, std::memory_order_relaxed);
            });

    if (ok)
    {
      ++accepted;
    }
    else
    {
      ++rejected;
    }
  }

  pool.wait_idle();

  EXPECT_GE(accepted, 1);
  EXPECT_GE(rejected, 1);
  EXPECT_EQ(completed.load(std::memory_order_relaxed), accepted);

  pool.shutdown();
}

TEST(StressTest, MetricsAndStatsRemainConsistentAfterLoad)
{
  vix::threadpool::ThreadPool pool(4);

  constexpr int taskCount = 200;
  std::atomic<int> counter{0};

  for (int i = 0; i < taskCount; ++i)
  {
    pool.post(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  pool.wait_idle();

  const vix::threadpool::ThreadPoolMetrics metrics = pool.metrics();
  const vix::threadpool::ThreadPoolStats stats = pool.stats();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), taskCount);
  EXPECT_EQ(metrics.pending_tasks, std::size_t{0});
  EXPECT_GE(metrics.completed_tasks, std::uint64_t{taskCount});
  EXPECT_GE(stats.completed_tasks, std::uint64_t{taskCount});
  EXPECT_EQ(metrics.failed_tasks, std::uint64_t{0});

  pool.shutdown();
}

TEST(StressTest, ShutdownWhileTasksAreRunningIsSafe)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> started{0};

  for (int i = 0; i < 20; ++i)
  {
    pool.post(
        [&started]()
        {
          started.fetch_add(1, std::memory_order_relaxed);
          std::this_thread::sleep_for(std::chrono::milliseconds{5});
        });
  }

  EXPECT_TRUE(
      wait_until_true(
          [&started]()
          {
            return started.load(std::memory_order_relaxed) > 0;
          }));

  pool.shutdown();

  EXPECT_FALSE(pool.running());
  EXPECT_GE(started.load(std::memory_order_relaxed), 1);
}
