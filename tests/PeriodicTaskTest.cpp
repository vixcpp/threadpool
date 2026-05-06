/**
 *
 * @file PeriodicTaskTest.cpp
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
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/InlineExecutor.hpp>
#include <vix/threadpool/PeriodicTask.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/ThreadPool.hpp>

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

TEST(PeriodicTaskTest, DefaultConfigIsValid)
{
  const vix::threadpool::PeriodicTaskConfig config;

  EXPECT_EQ(config.interval, std::chrono::milliseconds{1000});
  EXPECT_FALSE(config.run_immediately);
  EXPECT_TRUE(config.stop_on_post_failure);
  EXPECT_EQ(config.task_options.priority, vix::threadpool::TaskPriority::normal);
}

TEST(PeriodicTaskTest, EveryCreatesConfigWithInterval)
{
  const vix::threadpool::PeriodicTaskConfig config =
      vix::threadpool::PeriodicTaskConfig::every(
          std::chrono::milliseconds{250});

  EXPECT_EQ(config.interval, std::chrono::milliseconds{250});
}

TEST(PeriodicTaskTest, NormalizeIntervalTurnsNonPositiveIntoOneMillisecond)
{
  EXPECT_EQ(
      vix::threadpool::PeriodicTaskConfig::normalize_interval(
          std::chrono::milliseconds{0}),
      std::chrono::milliseconds{1});

  EXPECT_EQ(
      vix::threadpool::PeriodicTaskConfig::normalize_interval(
          std::chrono::milliseconds{-10}),
      std::chrono::milliseconds{1});
}

TEST(PeriodicTaskTest, NormalizedConfigFixesInvalidInterval)
{
  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{0};

  const vix::threadpool::PeriodicTaskConfig normalized = config.normalized();

  EXPECT_EQ(normalized.interval, std::chrono::milliseconds{1});
}

TEST(PeriodicTaskTest, EmptyPeriodicTaskDoesNotStart)
{
  vix::threadpool::PeriodicTask task;

  EXPECT_FALSE(task.running());
  EXPECT_FALSE(task.joinable());
  EXPECT_FALSE(task.start());
  EXPECT_EQ(task.submitted_ticks(), std::uint64_t{0});
  EXPECT_EQ(task.failed_posts(), std::uint64_t{0});
}

TEST(PeriodicTaskTest, StartsAndSubmitsTicksToInlineExecutor)
{
  vix::threadpool::InlineExecutor executor;

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{10};
  config.run_immediately = true;

  vix::threadpool::PeriodicTask task(
      executor,
      [&ticks]()
      {
        ticks.fetch_add(1, std::memory_order_relaxed);
      },
      config);

  ASSERT_TRUE(task.start());

  EXPECT_TRUE(
      wait_until_true(
          [&ticks]()
          {
            return ticks.load(std::memory_order_relaxed) >= 2;
          }));

  task.stop();
  task.join();

  EXPECT_FALSE(task.running());
  EXPECT_GE(task.submitted_ticks(), std::uint64_t{2});
  EXPECT_EQ(task.failed_posts(), std::uint64_t{0});
  EXPECT_GE(ticks.load(std::memory_order_relaxed), 2);
}

TEST(PeriodicTaskTest, StartIsIdempotentWhileRunning)
{
  vix::threadpool::InlineExecutor executor;

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{10};

  vix::threadpool::PeriodicTask task(
      executor,
      [&ticks]()
      {
        ticks.fetch_add(1, std::memory_order_relaxed);
      },
      config);

  EXPECT_TRUE(task.start());
  EXPECT_FALSE(task.start());

  task.stop();
  task.join();
}

TEST(PeriodicTaskTest, StopBeforeStartIsSafe)
{
  vix::threadpool::InlineExecutor executor;

  vix::threadpool::PeriodicTask task(
      executor,
      []() {});

  task.stop();
  task.join();

  EXPECT_FALSE(task.running());
}

TEST(PeriodicTaskTest, StopsOnPostFailureWhenConfigured)
{
  vix::threadpool::InlineExecutor executor;
  executor.shutdown();

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{10};
  config.run_immediately = true;
  config.stop_on_post_failure = true;

  vix::threadpool::PeriodicTask task(
      executor,
      [&ticks]()
      {
        ticks.fetch_add(1, std::memory_order_relaxed);
      },
      config);

  ASSERT_TRUE(task.start());

  EXPECT_TRUE(
      wait_until_true(
          [&task]()
          {
            return !task.running() || task.failed_posts() > 0;
          }));

  task.stop();
  task.join();

  EXPECT_GE(task.failed_posts(), std::uint64_t{1});
  EXPECT_EQ(ticks.load(std::memory_order_relaxed), 0);
}

TEST(PeriodicTaskTest, CanContinueAfterPostFailureWhenConfigured)
{
  vix::threadpool::InlineExecutor executor;
  executor.shutdown();

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{10};
  config.run_immediately = true;
  config.stop_on_post_failure = false;

  vix::threadpool::PeriodicTask task(
      executor,
      []() {},
      config);

  ASSERT_TRUE(task.start());

  EXPECT_TRUE(
      wait_until_true(
          [&task]()
          {
            return task.failed_posts() >= 2;
          }));

  EXPECT_TRUE(task.running());

  task.stop();
  task.join();
}

TEST(PeriodicTaskTest, WorksWithThreadPoolExecutor)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{10};
  config.run_immediately = true;

  vix::threadpool::PeriodicTask task(
      pool,
      [&ticks]()
      {
        ticks.fetch_add(1, std::memory_order_relaxed);
      },
      config);

  ASSERT_TRUE(task.start());

  EXPECT_TRUE(
      wait_until_true(
          [&ticks]()
          {
            return ticks.load(std::memory_order_relaxed) >= 2;
          }));

  task.stop();
  task.join();

  pool.wait_idle();

  EXPECT_GE(task.submitted_ticks(), std::uint64_t{2});
  EXPECT_GE(ticks.load(std::memory_order_relaxed), 2);

  pool.shutdown();
}
