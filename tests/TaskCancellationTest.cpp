/**
 *
 * @file TaskCancellationTest.cpp
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
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/CancellationSource.hpp>
#include <vix/threadpool/CancellationToken.hpp>
#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/ThreadPool.hpp>

TEST(TaskCancellationTest, DefaultTokenCannotCancel)
{
  const vix::threadpool::CancellationToken token;

  EXPECT_FALSE(token.can_cancel());
  EXPECT_FALSE(token.cancelled());
  EXPECT_FALSE(token.is_cancelled());
  EXPECT_FALSE(token.stop_requested());
  EXPECT_TRUE(token.can_continue());
}

TEST(TaskCancellationTest, SourceTokenObservesCancellation)
{
  vix::threadpool::CancellationSource source;
  const vix::threadpool::CancellationToken token = source.token();

  EXPECT_TRUE(token.can_cancel());
  EXPECT_FALSE(token.cancelled());

  source.request_cancel();

  EXPECT_TRUE(source.cancelled());
  EXPECT_TRUE(source.is_cancelled());
  EXPECT_TRUE(token.cancelled());
  EXPECT_TRUE(token.is_cancelled());
  EXPECT_TRUE(token.stop_requested());
  EXPECT_FALSE(token.can_continue());
}

TEST(TaskCancellationTest, SourceResetDoesNotCancelOldToken)
{
  vix::threadpool::CancellationSource source;

  const vix::threadpool::CancellationToken oldToken = source.token();

  source.reset();

  const vix::threadpool::CancellationToken newToken = source.token();

  source.request_cancel();

  EXPECT_FALSE(oldToken.cancelled());
  EXPECT_TRUE(newToken.cancelled());
}

TEST(TaskCancellationTest, TokenResetClearsState)
{
  vix::threadpool::CancellationSource source;
  vix::threadpool::CancellationToken token = source.token();

  EXPECT_TRUE(token.can_cancel());

  token.reset();

  EXPECT_FALSE(token.can_cancel());
  EXPECT_FALSE(token.cancelled());
}

TEST(TaskCancellationTest, TaskSkipsExecutionWhenCancelledBeforeRun)
{
  std::atomic<bool> executed{false};

  vix::threadpool::CancellationSource source;

  vix::threadpool::TaskOptions options;
  options.set_cancellation(source.token());

  source.request_cancel();

  vix::threadpool::Task task(
      vix::threadpool::TaskId{1},
      vix::threadpool::TaskFunction(
          [&executed]()
          {
            executed.store(true, std::memory_order_relaxed);
          }),
      options,
      1);

  const vix::threadpool::TaskResult result = task.run();

  EXPECT_EQ(result, vix::threadpool::TaskResult::cancelled);
  EXPECT_EQ(task.status(), vix::threadpool::TaskStatus::cancelled);
  EXPECT_EQ(task.result(), vix::threadpool::TaskResult::cancelled);
  EXPECT_FALSE(executed.load(std::memory_order_relaxed));
}

TEST(TaskCancellationTest, TaskReportsCancelledWhenTokenIsCancelledDuringRun)
{
  std::atomic<bool> executed{false};

  vix::threadpool::CancellationSource source;

  vix::threadpool::TaskOptions options;
  options.set_cancellation(source.token());

  vix::threadpool::Task task(
      vix::threadpool::TaskId{1},
      vix::threadpool::TaskFunction(
          [&executed, &source]()
          {
            executed.store(true, std::memory_order_relaxed);
            source.request_cancel();
          }),
      options,
      1);

  const vix::threadpool::TaskResult result = task.run();

  EXPECT_TRUE(executed.load(std::memory_order_relaxed));
  EXPECT_EQ(result, vix::threadpool::TaskResult::cancelled);
  EXPECT_EQ(task.status(), vix::threadpool::TaskStatus::cancelled);
  EXPECT_EQ(task.result(), vix::threadpool::TaskResult::cancelled);
}

TEST(TaskCancellationTest, ThreadPoolHandleCanRequestCancellation)
{
  vix::threadpool::ThreadPool pool(1);

  auto handle =
      pool.handle(
          []()
          {
            return 42;
          });

  handle.cancel();

  EXPECT_TRUE(handle.cancelled());

  pool.wait_idle();
  pool.shutdown();
}

TEST(TaskCancellationTest, CancelledQueuedTaskCanBeObservedAsCancelled)
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

  blocker.get();

  try
  {
    (void)handle.get();
  }
  catch (...)
  {
  }

  EXPECT_TRUE(handle.cancelled());

  pool.shutdown();
}
