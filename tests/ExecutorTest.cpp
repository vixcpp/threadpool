/**
 *
 * @file ExecutorTest.cpp
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
#include <cstdint>
#include <functional>
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/Executor.hpp>
#include <vix/threadpool/InlineExecutor.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/ThreadPoolExecutor.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>

namespace
{
  bool wait_until_true(std::function<bool()> predicate)
  {
    for (int i = 0; i < 100; ++i)
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

TEST(ExecutorTest, InlineExecutorRunsPostedTaskImmediately)
{
  vix::threadpool::InlineExecutor executor;

  std::atomic<int> counter{0};

  const bool accepted =
      executor.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  EXPECT_TRUE(accepted);
  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
  EXPECT_TRUE(executor.idle());
  EXPECT_TRUE(executor.running());

  const vix::threadpool::ThreadPoolMetrics metrics = executor.metrics();

  EXPECT_EQ(metrics.submitted_tasks, std::uint64_t{1});
  EXPECT_EQ(metrics.completed_tasks, std::uint64_t{1});
  EXPECT_EQ(metrics.rejected_tasks, std::uint64_t{0});
}

TEST(ExecutorTest, InlineExecutorRejectsEmptyTask)
{
  vix::threadpool::InlineExecutor executor;

  const bool accepted = executor.post(vix::threadpool::Executor::Task{});

  EXPECT_FALSE(accepted);

  const vix::threadpool::ThreadPoolMetrics metrics = executor.metrics();

  EXPECT_EQ(metrics.submitted_tasks, std::uint64_t{1});
  EXPECT_EQ(metrics.rejected_tasks, std::uint64_t{1});
}

TEST(ExecutorTest, InlineExecutorRejectsAfterShutdown)
{
  vix::threadpool::InlineExecutor executor;

  executor.shutdown();

  EXPECT_FALSE(executor.running());

  const bool accepted =
      executor.post(
          []() {});

  EXPECT_FALSE(accepted);

  const vix::threadpool::ThreadPoolMetrics metrics = executor.metrics();

  EXPECT_EQ(metrics.rejected_tasks, std::uint64_t{1});
}

TEST(ExecutorTest, InlineExecutorAllowsAfterStopWhenOptionIsSet)
{
  vix::threadpool::InlineExecutor executor;

  executor.shutdown();

  std::atomic<int> counter{0};

  vix::threadpool::TaskOptions options;
  options.set_allow_after_stop(true);

  const bool accepted =
      executor.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          },
          options);

  EXPECT_TRUE(accepted);
  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
}

TEST(ExecutorTest, InlineExecutorRecordsFailedTask)
{
  vix::threadpool::InlineExecutor executor;

  const bool accepted =
      executor.post(
          []()
          {
            throw std::runtime_error{"boom"};
          });

  EXPECT_FALSE(accepted);

  const vix::threadpool::ThreadPoolMetrics metrics = executor.metrics();
  const vix::threadpool::ThreadPoolStats stats = executor.stats();

  EXPECT_EQ(metrics.failed_tasks, std::uint64_t{1});
  EXPECT_EQ(stats.failed_tasks, std::uint64_t{1});
}

TEST(ExecutorTest, ExecutorRefCanPostToInlineExecutor)
{
  vix::threadpool::InlineExecutor executor;
  vix::threadpool::ExecutorRef ref(executor);

  std::atomic<int> counter{0};

  EXPECT_TRUE(ref.valid());
  EXPECT_TRUE(static_cast<bool>(ref));

  const bool accepted =
      ref.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  EXPECT_TRUE(accepted);
  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
  EXPECT_TRUE(ref.idle());
  EXPECT_TRUE(ref.running());
}

TEST(ExecutorTest, EmptyExecutorRefIsSafe)
{
  const vix::threadpool::ExecutorRef ref;

  EXPECT_FALSE(ref.valid());
  EXPECT_FALSE(static_cast<bool>(ref));
  EXPECT_FALSE(ref.post([]() {}));
  EXPECT_TRUE(ref.idle());
  EXPECT_FALSE(ref.running());

  const vix::threadpool::ThreadPoolMetrics metrics = ref.metrics();
  const vix::threadpool::ThreadPoolStats stats = ref.stats();

  EXPECT_EQ(metrics.submitted_tasks, std::uint64_t{0});
  EXPECT_EQ(stats.submitted_tasks(), std::uint64_t{0});
}

TEST(ExecutorTest, ThreadPoolExecutorCanPostToThreadPool)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::ThreadPoolExecutor executor(pool);

  std::atomic<int> counter{0};

  EXPECT_TRUE(executor.valid());
  EXPECT_TRUE(static_cast<bool>(executor));

  const bool accepted =
      executor.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  EXPECT_TRUE(accepted);

  ASSERT_TRUE(
      wait_until_true(
          [&counter]()
          {
            return counter.load(std::memory_order_relaxed) == 1;
          }));

  executor.wait_idle();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
  EXPECT_TRUE(executor.idle());
  EXPECT_TRUE(executor.running());

  executor.shutdown();

  EXPECT_FALSE(executor.running());
}

TEST(ExecutorTest, ThreadPoolExecutorReturnsEmptyMetricsWhenUnbound)
{
  vix::threadpool::ThreadPoolExecutor executor;

  EXPECT_FALSE(executor.valid());
  EXPECT_FALSE(static_cast<bool>(executor));
  EXPECT_EQ(executor.pool(), nullptr);
  EXPECT_FALSE(executor.post([]() {}));
  EXPECT_FALSE(executor.running());
  EXPECT_TRUE(executor.idle());

  const vix::threadpool::ThreadPoolMetrics metrics = executor.metrics();
  const vix::threadpool::ThreadPoolStats stats = executor.stats();

  EXPECT_EQ(metrics.worker_count, std::size_t{0});
  EXPECT_EQ(stats.submitted_tasks(), std::uint64_t{0});
}

TEST(ExecutorTest, ThreadPoolExecutorCanResetBinding)
{
  vix::threadpool::ThreadPool pool(1);
  vix::threadpool::ThreadPoolExecutor executor;

  executor.reset(pool);

  EXPECT_TRUE(executor.valid());
  EXPECT_EQ(executor.pool(), &pool);

  executor.reset();

  EXPECT_FALSE(executor.valid());
  EXPECT_EQ(executor.pool(), nullptr);

  pool.shutdown();
}
