/**
 *
 * @file ThreadPoolTest.cpp
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
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/TaskHandle.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/ThreadPoolConfig.hpp>
#include <vix/threadpool/ThreadPoolError.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>
#include <vix/threadpool/Timeout.hpp>

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

TEST(ThreadPoolTest, DefaultPoolStartsAutomatically)
{
  vix::threadpool::ThreadPool pool;

  EXPECT_TRUE(pool.running());
  EXPECT_GE(pool.thread_count(), std::size_t{1});

  pool.shutdown();

  EXPECT_FALSE(pool.running());
}

TEST(ThreadPoolTest, FixedThreadCountPoolUsesRequestedThreadCount)
{
  vix::threadpool::ThreadPool pool(2);

  EXPECT_TRUE(pool.running());
  EXPECT_EQ(pool.thread_count(), std::size_t{2});

  pool.shutdown();
}

TEST(ThreadPoolTest, ConfigConstructorNormalizesThreadCount)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 0;
  config.max_thread_count = 0;

  vix::threadpool::ThreadPool pool(config);

  EXPECT_TRUE(pool.running());
  EXPECT_GE(pool.thread_count(), std::size_t{1});

  pool.shutdown();
}

TEST(ThreadPoolTest, PostExecutesFireAndForgetTask)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  const bool accepted =
      pool.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  EXPECT_TRUE(accepted);

  pool.wait_idle();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);

  pool.shutdown();
}

TEST(ThreadPoolTest, PostRejectsEmptyTask)
{
  vix::threadpool::ThreadPool pool(1);

  EXPECT_FALSE(pool.post(vix::threadpool::Executor::Task{}));

  pool.shutdown();
}

TEST(ThreadPoolTest, SubmitReturnsFutureResult)
{
  vix::threadpool::ThreadPool pool(2);

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  EXPECT_EQ(future.get(), 42);
  EXPECT_EQ(future.status(), vix::threadpool::TaskStatus::completed);
  EXPECT_EQ(future.result(), vix::threadpool::TaskResult::success);

  pool.shutdown();
}

TEST(ThreadPoolTest, SubmitSupportsVoidTask)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  auto future =
      pool.submit(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  future.get();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
  EXPECT_EQ(future.status(), vix::threadpool::TaskStatus::completed);

  pool.shutdown();
}

TEST(ThreadPoolTest, SubmitPropagatesExceptionThroughFuture)
{
  vix::threadpool::ThreadPool pool(2);

  auto future =
      pool.submit(
          []() -> int
          {
            throw std::runtime_error{"task failed"};
          });

  EXPECT_THROW((void)future.get(), std::runtime_error);
  EXPECT_EQ(future.status(), vix::threadpool::TaskStatus::failed);
  EXPECT_EQ(future.result(), vix::threadpool::TaskResult::failure);

  pool.shutdown();
}

TEST(ThreadPoolTest, SubmitAfterShutdownReturnsRejectedFuture)
{
  vix::threadpool::ThreadPool pool(1);

  pool.shutdown();

  auto future =
      pool.submit(
          []()
          {
            return 7;
          });

  EXPECT_TRUE(future.ready());
  EXPECT_EQ(future.status(), vix::threadpool::TaskStatus::rejected);
  EXPECT_EQ(future.result(), vix::threadpool::TaskResult::rejected);
  EXPECT_EQ(future.error(), vix::threadpool::ThreadPoolErrc::rejected);
  EXPECT_THROW((void)future.get(), std::system_error);
}

TEST(ThreadPoolTest, HandleReturnsTaskIdAndFuture)
{
  vix::threadpool::ThreadPool pool(2);

  auto handle =
      pool.handle(
          []()
          {
            return std::string{"vix"};
          });

  EXPECT_TRUE(handle.valid());
  EXPECT_NE(handle.id(), vix::threadpool::invalid_task_id);

  EXPECT_EQ(handle.get(), "vix");
  EXPECT_EQ(handle.status(), vix::threadpool::TaskStatus::completed);

  pool.shutdown();
}

TEST(ThreadPoolTest, HandleCanRequestCancellation)
{
  vix::threadpool::ThreadPool pool(1);

  auto blocker =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
          });

  auto handle =
      pool.handle(
          []()
          {
            return 7;
          });

  handle.cancel();

  EXPECT_TRUE(handle.cancelled());

  blocker.get();

  try
  {
    (void)handle.get();
  }
  catch (...)
  {
  }

  pool.shutdown();
}

TEST(ThreadPoolTest, WaitIdleWaitsForPendingWork)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  for (int i = 0; i < 8; ++i)
  {
    pool.post(
        [&counter]()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{5});
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  pool.wait_idle();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 8);
  EXPECT_TRUE(pool.idle());

  pool.shutdown();
}

TEST(ThreadPoolTest, MetricsReflectExecutedTasks)
{
  vix::threadpool::ThreadPool pool(2);

  for (int i = 0; i < 4; ++i)
  {
    pool.post(
        []() {});
  }

  pool.wait_idle();

  const vix::threadpool::ThreadPoolMetrics metrics = pool.metrics();

  EXPECT_EQ(metrics.worker_count, std::size_t{2});
  EXPECT_EQ(metrics.pending_tasks, std::size_t{0});
  EXPECT_GE(metrics.completed_tasks, std::uint64_t{4});
  EXPECT_TRUE(metrics.idle());

  pool.shutdown();
}

TEST(ThreadPoolTest, StatsReflectExecutedTasks)
{
  vix::threadpool::ThreadPool pool(2);

  auto first =
      pool.submit(
          []()
          {
            return 10;
          });

  auto second =
      pool.submit(
          []()
          {
            return 32;
          });

  EXPECT_EQ(first.get() + second.get(), 42);

  const vix::threadpool::ThreadPoolStats stats = pool.stats();

  EXPECT_GE(stats.accepted_tasks, std::uint64_t{2});
  EXPECT_GE(stats.completed_tasks, std::uint64_t{2});
  EXPECT_EQ(stats.failed_tasks, std::uint64_t{0});

  pool.shutdown();
}

TEST(ThreadPoolTest, ClearRemovesQueuedTasks)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 1;
  config.max_thread_count = 1;
  config.max_queue_size = 32;

  vix::threadpool::ThreadPool pool(config);

  auto blocker =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{80});
          });

  for (int i = 0; i < 10; ++i)
  {
    pool.post(
        []()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{10});
        });
  }

  const bool sawPending =
      wait_until_true(
          [&pool]()
          {
            return pool.pending() > 0;
          });

  EXPECT_TRUE(sawPending);

  const std::size_t removed = pool.clear();

  EXPECT_GE(removed, std::size_t{0});

  blocker.get();
  pool.shutdown();
}

TEST(ThreadPoolTest, DefaultTimeoutIsMergedIntoTaskOptions)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 1;
  config.max_thread_count = 1;
  config.default_timeout = std::chrono::milliseconds{1};

  vix::threadpool::ThreadPool pool(config);

  auto future =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
            return 42;
          });

  try
  {
    (void)future.get();
  }
  catch (...)
  {
  }

  EXPECT_TRUE(
      future.status() == vix::threadpool::TaskStatus::completed ||
      future.status() == vix::threadpool::TaskStatus::timed_out);

  pool.shutdown();
}

TEST(ThreadPoolTest, NextTaskIdReturnsUniqueIds)
{
  vix::threadpool::ThreadPool pool(1);

  const vix::threadpool::TaskId first = pool.next_task_id();
  const vix::threadpool::TaskId second = pool.next_task_id();

  EXPECT_NE(first, vix::threadpool::invalid_task_id);
  EXPECT_NE(second, vix::threadpool::invalid_task_id);
  EXPECT_NE(first, second);

  pool.shutdown();
}

TEST(ThreadPoolTest, PostAfterShutdownIsRejected)
{
  vix::threadpool::ThreadPool pool(1);

  pool.shutdown();

  const bool accepted =
      pool.post(
          []() {});

  EXPECT_FALSE(accepted);
}
