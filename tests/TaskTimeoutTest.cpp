/**
 *
 * @file TaskTimeoutTest.cpp
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
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/Deadline.hpp>
#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/Timeout.hpp>

TEST(TaskTimeoutTest, DefaultTimeoutIsDisabled)
{
  const vix::threadpool::Timeout timeout;

  EXPECT_FALSE(timeout.enabled());
  EXPECT_TRUE(timeout.disabled_value());
  EXPECT_EQ(timeout.count(), 0);
  EXPECT_FALSE(timeout.expired(std::chrono::milliseconds{1000}));
}

TEST(TaskTimeoutTest, NegativeTimeoutIsNormalizedToZero)
{
  const vix::threadpool::Timeout timeout =
      vix::threadpool::Timeout::milliseconds(-10);

  EXPECT_FALSE(timeout.enabled());
  EXPECT_EQ(timeout.count(), 0);
}

TEST(TaskTimeoutTest, TimeoutExpiresWhenElapsedIsGreaterThanValue)
{
  const vix::threadpool::Timeout timeout =
      vix::threadpool::Timeout::milliseconds(50);

  EXPECT_TRUE(timeout.enabled());
  EXPECT_FALSE(timeout.expired(std::chrono::milliseconds{50}));
  EXPECT_TRUE(timeout.expired(std::chrono::milliseconds{51}));
}

TEST(TaskTimeoutTest, TimeoutSecondsFactoryWorks)
{
  const vix::threadpool::Timeout timeout =
      vix::threadpool::Timeout::seconds(2);

  EXPECT_TRUE(timeout.enabled());
  EXPECT_EQ(timeout.count(), 2000);
}

TEST(TaskTimeoutTest, DefaultDeadlineIsDisabled)
{
  const vix::threadpool::Deadline deadline;

  EXPECT_FALSE(deadline.enabled());
  EXPECT_TRUE(deadline.disabled_value());
  EXPECT_FALSE(deadline.expired());
  EXPECT_EQ(deadline.remaining(), vix::threadpool::Deadline::clock::duration::zero());
}

TEST(TaskTimeoutTest, DeadlineFromDisabledTimeoutIsDisabled)
{
  const vix::threadpool::Deadline deadline =
      vix::threadpool::Deadline::from_timeout(
          vix::threadpool::Timeout::disabled());

  EXPECT_FALSE(deadline.enabled());
  EXPECT_FALSE(deadline.expired());
}

TEST(TaskTimeoutTest, DeadlineAfterCanExpire)
{
  const vix::threadpool::Deadline deadline =
      vix::threadpool::Deadline::after(std::chrono::milliseconds{1});

  EXPECT_TRUE(deadline.enabled());

  std::this_thread::sleep_for(std::chrono::milliseconds{5});

  EXPECT_TRUE(deadline.expired());
}

TEST(TaskTimeoutTest, TaskSkipsExecutionWhenDeadlineAlreadyExpired)
{
  bool executed = false;

  vix::threadpool::TaskOptions options;
  options.set_deadline(
      vix::threadpool::Deadline::after(std::chrono::milliseconds{-1}));

  vix::threadpool::Task task(
      vix::threadpool::TaskId{1},
      vix::threadpool::TaskFunction(
          [&executed]()
          {
            executed = true;
          }),
      options,
      1);

  const vix::threadpool::TaskResult result = task.run();

  EXPECT_FALSE(executed);
  EXPECT_EQ(result, vix::threadpool::TaskResult::timeout);
  EXPECT_EQ(task.status(), vix::threadpool::TaskStatus::timed_out);
  EXPECT_EQ(task.result(), vix::threadpool::TaskResult::timeout);
}

TEST(TaskTimeoutTest, TaskReportsTimeoutAfterExecutionWhenTimeoutExceeded)
{
  bool executed = false;

  vix::threadpool::TaskOptions options;
  options.set_timeout(vix::threadpool::Timeout::milliseconds(1));

  vix::threadpool::Task task(
      vix::threadpool::TaskId{1},
      vix::threadpool::TaskFunction(
          [&executed]()
          {
            executed = true;
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
          }),
      options,
      1);

  const vix::threadpool::TaskResult result = task.run();

  EXPECT_TRUE(executed);
  EXPECT_EQ(result, vix::threadpool::TaskResult::timeout);
  EXPECT_EQ(task.status(), vix::threadpool::TaskStatus::timed_out);
  EXPECT_EQ(task.result(), vix::threadpool::TaskResult::timeout);
}

TEST(TaskTimeoutTest, TaskCompletesWhenTimeoutIsNotExceeded)
{
  bool executed = false;

  vix::threadpool::TaskOptions options;
  options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

  vix::threadpool::Task task(
      vix::threadpool::TaskId{1},
      vix::threadpool::TaskFunction(
          [&executed]()
          {
            executed = true;
          }),
      options,
      1);

  const vix::threadpool::TaskResult result = task.run();

  EXPECT_TRUE(executed);
  EXPECT_EQ(result, vix::threadpool::TaskResult::success);
  EXPECT_EQ(task.status(), vix::threadpool::TaskStatus::completed);
  EXPECT_EQ(task.result(), vix::threadpool::TaskResult::success);
}

TEST(TaskTimeoutTest, ThreadPoolFutureCanObserveTimeoutStatus)
{
  vix::threadpool::ThreadPool pool(1);

  vix::threadpool::TaskOptions options;
  options.set_timeout(vix::threadpool::Timeout::milliseconds(1));

  auto future =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
            return 42;
          },
          options);

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
