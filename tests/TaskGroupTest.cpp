/**
 *
 * @file TaskGroupTest.cpp
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
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/TaskGroup.hpp>
#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>

TEST(TaskGroupTest, NewGroupIsEmpty)
{
  const vix::threadpool::TaskGroup group;

  EXPECT_TRUE(group.empty());
  EXPECT_TRUE(group.done());
  EXPECT_FALSE(group.closed());
  EXPECT_FALSE(group.cancelled());
  EXPECT_EQ(group.total_tasks(), std::uint64_t{0});
  EXPECT_EQ(group.pending_tasks(), std::uint64_t{0});
}

TEST(TaskGroupTest, AddTaskRegistersValidTaskId)
{
  vix::threadpool::TaskGroup group;

  EXPECT_TRUE(group.add_task(vix::threadpool::TaskId{1}));
  EXPECT_TRUE(group.add_task(vix::threadpool::TaskId{2}));

  EXPECT_FALSE(group.empty());
  EXPECT_FALSE(group.done());
  EXPECT_EQ(group.total_tasks(), std::uint64_t{2});
  EXPECT_EQ(group.pending_tasks(), std::uint64_t{2});

  const std::vector<vix::threadpool::TaskId> ids = group.task_ids();

  ASSERT_EQ(ids.size(), std::size_t{2});
  EXPECT_EQ(ids[0], vix::threadpool::TaskId{1});
  EXPECT_EQ(ids[1], vix::threadpool::TaskId{2});
}

TEST(TaskGroupTest, AddTaskRejectsInvalidTaskId)
{
  vix::threadpool::TaskGroup group;

  EXPECT_FALSE(group.add_task(vix::threadpool::invalid_task_id));

  EXPECT_TRUE(group.empty());
  EXPECT_EQ(group.total_tasks(), std::uint64_t{0});
  EXPECT_EQ(group.pending_tasks(), std::uint64_t{0});
}

TEST(TaskGroupTest, AddTaskRejectsWhenClosed)
{
  vix::threadpool::TaskGroup group;

  group.close();

  EXPECT_TRUE(group.closed());
  EXPECT_FALSE(group.add_task(vix::threadpool::TaskId{1}));
  EXPECT_TRUE(group.empty());
}

TEST(TaskGroupTest, FinishTaskUpdatesCompletedCounters)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));

  group.finish_task(
      vix::threadpool::TaskStatus::completed,
      vix::threadpool::TaskResult::success);

  EXPECT_TRUE(group.done());
  EXPECT_EQ(group.pending_tasks(), std::uint64_t{0});
  EXPECT_EQ(group.completed_tasks(), std::uint64_t{1});
  EXPECT_EQ(group.failed_tasks(), std::uint64_t{0});
  EXPECT_FALSE(group.has_failure());
  EXPECT_FALSE(group.has_error());
}

TEST(TaskGroupTest, FinishTaskUpdatesFailureCounters)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));

  group.finish_task(
      vix::threadpool::TaskStatus::failed,
      vix::threadpool::TaskResult::failure,
      std::make_exception_ptr(std::runtime_error{"boom"}));

  EXPECT_TRUE(group.done());
  EXPECT_EQ(group.failed_tasks(), std::uint64_t{1});
  EXPECT_TRUE(group.has_failure());
  EXPECT_TRUE(group.has_error());
  EXPECT_NE(group.first_exception(), nullptr);
}

TEST(TaskGroupTest, FinishTaskUpdatesCancelledCounters)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));

  group.finish_task(
      vix::threadpool::TaskStatus::cancelled,
      vix::threadpool::TaskResult::cancelled);

  EXPECT_TRUE(group.done());
  EXPECT_EQ(group.cancelled_tasks(), std::uint64_t{1});
  EXPECT_TRUE(group.has_error());
}

TEST(TaskGroupTest, FinishTaskUpdatesTimedOutCounters)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));

  group.finish_task(
      vix::threadpool::TaskStatus::timed_out,
      vix::threadpool::TaskResult::timeout);

  EXPECT_TRUE(group.done());
  EXPECT_EQ(group.timed_out_tasks(), std::uint64_t{1});
  EXPECT_TRUE(group.has_error());
}

TEST(TaskGroupTest, FinishTaskUpdatesRejectedCounters)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));

  group.finish_task(
      vix::threadpool::TaskStatus::rejected,
      vix::threadpool::TaskResult::rejected);

  EXPECT_TRUE(group.done());
  EXPECT_EQ(group.rejected_tasks(), std::uint64_t{1});
  EXPECT_TRUE(group.has_error());
}

TEST(TaskGroupTest, WaitBlocksUntilAllTasksFinish)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));
  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{2}));

  std::thread first(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});

        group.finish_task(
            vix::threadpool::TaskStatus::completed,
            vix::threadpool::TaskResult::success);
      });

  std::thread second(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});

        group.finish_task(
            vix::threadpool::TaskStatus::completed,
            vix::threadpool::TaskResult::success);
      });

  group.wait();

  first.join();
  second.join();

  EXPECT_TRUE(group.done());
  EXPECT_EQ(group.completed_tasks(), std::uint64_t{2});
}

TEST(TaskGroupTest, WaitAndRethrowRethrowsFirstException)
{
  vix::threadpool::TaskGroup group;

  ASSERT_TRUE(group.add_task(vix::threadpool::TaskId{1}));

  group.finish_task(
      vix::threadpool::TaskStatus::failed,
      vix::threadpool::TaskResult::failure,
      std::make_exception_ptr(std::runtime_error{"task failed"}));

  EXPECT_THROW(group.wait_and_rethrow(), std::runtime_error);
}

TEST(TaskGroupTest, CancelRequestsSharedCancellation)
{
  vix::threadpool::TaskGroup group;

  const vix::threadpool::CancellationToken token =
      group.cancellation_token();

  EXPECT_TRUE(token.can_cancel());
  EXPECT_FALSE(token.cancelled());

  group.cancel();

  EXPECT_TRUE(group.cancelled());
  EXPECT_TRUE(token.cancelled());
}
