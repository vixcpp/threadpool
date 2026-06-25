/**
 *
 * @file ShutdownTest.cpp
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
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/ThreadPoolConfig.hpp>
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

TEST(ShutdownTest, ShutdownStopsPool)
{
  vix::threadpool::ThreadPool pool(2);

  EXPECT_TRUE(pool.running());

  pool.shutdown();

  EXPECT_FALSE(pool.running());
}

TEST(ShutdownTest, ShutdownIsIdempotent)
{
  vix::threadpool::ThreadPool pool(2);

  pool.shutdown();
  pool.shutdown();
  pool.shutdown();

  EXPECT_FALSE(pool.running());
}

TEST(ShutdownTest, PostAfterShutdownIsRejected)
{
  vix::threadpool::ThreadPool pool(1);

  pool.shutdown();

  const bool accepted =
      pool.post(
          []() {});

  EXPECT_FALSE(accepted);
}

TEST(ShutdownTest, SubmitAfterShutdownReturnsRejectedFuture)
{
  vix::threadpool::ThreadPool pool(1);

  pool.shutdown();

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  EXPECT_TRUE(future.ready());
  EXPECT_EQ(future.error(), vix::threadpool::ThreadPoolErrc::rejected);
  EXPECT_EQ(future.status(), vix::threadpool::TaskStatus::rejected);
  EXPECT_EQ(future.result(), vix::threadpool::TaskResult::rejected);
  EXPECT_THROW((void)future.get(), std::system_error);
}

TEST(ShutdownTest, WaitIdleBeforeShutdownWaitsForTasks)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i)
  {
    ASSERT_TRUE(pool.post(
        [&counter]()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{2});
          counter.fetch_add(1, std::memory_order_relaxed);
        }));
  }

  pool.wait_idle();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 10);
  EXPECT_TRUE(pool.idle());

  pool.shutdown();
}

TEST(ShutdownTest, ShutdownDrainsQueuedTasksWhenConfigured)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 1;
  config.max_thread_count = 1;
  config.max_queue_size = 32;
  config.drain_on_shutdown = true;

  vix::threadpool::ThreadPool pool(config);

  std::atomic<int> counter{0};

  for (int i = 0; i < 6; ++i)
  {
    EXPECT_TRUE(
        pool.post(
            [&counter]()
            {
              std::this_thread::sleep_for(std::chrono::milliseconds{2});
              counter.fetch_add(1, std::memory_order_relaxed);
            }));
  }

  pool.shutdown();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 6);
  EXPECT_FALSE(pool.running());
}

TEST(ShutdownTest, ShutdownWithoutDrainMayStopBeforeQueuedTasksFinish)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 1;
  config.max_thread_count = 1;
  config.max_queue_size = 64;
  config.drain_on_shutdown = false;

  vix::threadpool::ThreadPool pool(config);

  std::atomic<int> counter{0};

  for (int i = 0; i < 20; ++i)
  {
    ASSERT_TRUE(pool.post(
        [&counter]()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{5});
          counter.fetch_add(1, std::memory_order_relaxed);
        }));
  }

  pool.shutdown();

  EXPECT_FALSE(pool.running());
  EXPECT_LE(counter.load(std::memory_order_relaxed), 20);
}

TEST(ShutdownTest, AllowAfterStopOptionCanRunWhenSchedulerPolicyAllowsIt)
{
  vix::threadpool::ThreadPool pool(1);

  pool.shutdown();

  std::atomic<int> counter{0};

  vix::threadpool::TaskOptions options;
  options.set_allow_after_stop(true);

  const bool accepted =
      pool.post(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          },
          options);

  EXPECT_FALSE(accepted);
  EXPECT_EQ(counter.load(std::memory_order_relaxed), 0);
}

TEST(ShutdownTest, ClearBeforeShutdownRemovesPendingTasks)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 1;
  config.max_thread_count = 1;
  config.max_queue_size = 64;

  vix::threadpool::ThreadPool pool(config);

  std::promise<void> blockerStarted;
  auto blockerStartedFuture = blockerStarted.get_future();

  auto blocker =
      pool.submit(
          [&blockerStarted]()
          {
            blockerStarted.set_value();
            std::this_thread::sleep_for(std::chrono::milliseconds{80});
          });

  ASSERT_EQ(
      blockerStartedFuture.wait_for(std::chrono::seconds{1}),
      std::future_status::ready);

  for (int i = 0; i < 20; ++i)
  {
    ASSERT_TRUE(
        pool.post(
            []()
            {
              std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }));
  }

  const bool hasPending =
      wait_until_true(
          [&pool]()
          {
            return pool.pending() > 0;
          });

  EXPECT_TRUE(hasPending);

  const std::size_t removed = pool.clear();

  EXPECT_GE(removed, std::size_t{0});

  blocker.get();
  pool.shutdown();

  EXPECT_FALSE(pool.running());
}

TEST(ShutdownTest, MetricsRemainReadableAfterShutdown)
{
  vix::threadpool::ThreadPool pool(2);

  for (int i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(pool.post(
        []() {}));
  }

  pool.wait_idle();
  pool.shutdown();

  const vix::threadpool::ThreadPoolMetrics metrics = pool.metrics();
  const vix::threadpool::ThreadPoolStats stats = pool.stats();

  EXPECT_EQ(metrics.pending_tasks, std::size_t{0});
  EXPECT_GE(metrics.completed_tasks, std::uint64_t{4});
  EXPECT_GE(stats.completed_tasks, std::uint64_t{4});
}
