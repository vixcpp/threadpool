/**
 *
 * @file TaskQueueTest.cpp
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
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include <gtest/gtest.h>

#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/TaskQueue.hpp>
#include <vix/threadpool/TaskStatus.hpp>

namespace
{
  vix::threadpool::Task make_task(
      vix::threadpool::TaskId id,
      vix::threadpool::TaskPriority priority =
          vix::threadpool::TaskPriority::normal,
      std::uint64_t sequence = 0)
  {
    vix::threadpool::TaskOptions options;
    options.set_priority(priority);

    return vix::threadpool::Task(
        id,
        vix::threadpool::TaskFunction(
            []() {}),
        std::move(options),
        sequence);
  }
}

TEST(TaskQueueTest, DefaultQueueIsEmptyAndUnbounded)
{
  const vix::threadpool::TaskQueue queue;

  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.full());
  EXPECT_EQ(queue.size(), std::size_t{0});
  EXPECT_EQ(queue.max_size(), std::size_t{0});
  EXPECT_FALSE(queue.bounded());
}

TEST(TaskQueueTest, BoundedQueueReportsCapacity)
{
  const vix::threadpool::TaskQueue queue(2);

  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.full());
  EXPECT_EQ(queue.max_size(), std::size_t{2});
  EXPECT_TRUE(queue.bounded());
}

TEST(TaskQueueTest, PushAcceptsValidTask)
{
  vix::threadpool::TaskQueue queue;

  const bool accepted = queue.push(make_task(1));

  EXPECT_TRUE(accepted);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), std::size_t{1});
}

TEST(TaskQueueTest, PushRejectsInvalidTask)
{
  vix::threadpool::TaskQueue queue;

  const bool accepted = queue.push(vix::threadpool::Task{});

  EXPECT_FALSE(accepted);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), std::size_t{0});
}

TEST(TaskQueueTest, PushRejectsWhenQueueIsFull)
{
  vix::threadpool::TaskQueue queue(1);

  EXPECT_TRUE(queue.push(make_task(1)));
  EXPECT_TRUE(queue.full());

  EXPECT_FALSE(queue.push(make_task(2)));
  EXPECT_EQ(queue.size(), std::size_t{1});
}

TEST(TaskQueueTest, PopReturnsNulloptWhenEmpty)
{
  vix::threadpool::TaskQueue queue;

  std::optional<vix::threadpool::Task> task = queue.pop();

  EXPECT_FALSE(task.has_value());
}

TEST(TaskQueueTest, PopReturnsQueuedTask)
{
  vix::threadpool::TaskQueue queue;

  EXPECT_TRUE(queue.push(make_task(42)));

  std::optional<vix::threadpool::Task> task = queue.pop();

  ASSERT_TRUE(task.has_value());
  EXPECT_EQ(task->id(), vix::threadpool::TaskId{42});
  EXPECT_EQ(task->status(), vix::threadpool::TaskStatus::queued);
  EXPECT_TRUE(queue.empty());
}

TEST(TaskQueueTest, HigherPriorityTaskIsPoppedFirst)
{
  vix::threadpool::TaskQueue queue;

  EXPECT_TRUE(queue.push(make_task(
      1,
      vix::threadpool::TaskPriority::low,
      1)));

  EXPECT_TRUE(queue.push(make_task(
      2,
      vix::threadpool::TaskPriority::highest,
      2)));

  EXPECT_TRUE(queue.push(make_task(
      3,
      vix::threadpool::TaskPriority::normal,
      3)));

  std::optional<vix::threadpool::Task> first = queue.pop();
  std::optional<vix::threadpool::Task> second = queue.pop();
  std::optional<vix::threadpool::Task> third = queue.pop();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  ASSERT_TRUE(third.has_value());

  EXPECT_EQ(first->id(), vix::threadpool::TaskId{2});
  EXPECT_EQ(second->id(), vix::threadpool::TaskId{3});
  EXPECT_EQ(third->id(), vix::threadpool::TaskId{1});
}

TEST(TaskQueueTest, SamePriorityKeepsSequenceOrder)
{
  vix::threadpool::TaskQueue queue;

  EXPECT_TRUE(queue.push(make_task(
      1,
      vix::threadpool::TaskPriority::normal,
      10)));

  EXPECT_TRUE(queue.push(make_task(
      2,
      vix::threadpool::TaskPriority::normal,
      5)));

  EXPECT_TRUE(queue.push(make_task(
      3,
      vix::threadpool::TaskPriority::normal,
      20)));

  std::optional<vix::threadpool::Task> first = queue.pop();
  std::optional<vix::threadpool::Task> second = queue.pop();
  std::optional<vix::threadpool::Task> third = queue.pop();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  ASSERT_TRUE(third.has_value());

  EXPECT_EQ(first->id(), vix::threadpool::TaskId{2});
  EXPECT_EQ(second->id(), vix::threadpool::TaskId{1});
  EXPECT_EQ(third->id(), vix::threadpool::TaskId{3});
}

TEST(TaskQueueTest, PeekReturnsNextTaskWithoutRemovingIt)
{
  vix::threadpool::TaskQueue queue;

  EXPECT_TRUE(queue.push(make_task(
      1,
      vix::threadpool::TaskPriority::low,
      1)));

  EXPECT_TRUE(queue.push(make_task(
      2,
      vix::threadpool::TaskPriority::high,
      2)));

  const vix::threadpool::Task *task = queue.peek();

  ASSERT_NE(task, nullptr);
  EXPECT_EQ(task->id(), vix::threadpool::TaskId{2});
  EXPECT_EQ(queue.size(), std::size_t{2});
}

TEST(TaskQueueTest, ClearRemovesAllTasks)
{
  vix::threadpool::TaskQueue queue;

  EXPECT_TRUE(queue.push(make_task(1)));
  EXPECT_TRUE(queue.push(make_task(2)));
  EXPECT_TRUE(queue.push(make_task(3)));

  const std::size_t removed = queue.clear();

  EXPECT_EQ(removed, std::size_t{3});
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), std::size_t{0});
}

TEST(TaskQueueTest, SetMaxSizeUpdatesCapacity)
{
  vix::threadpool::TaskQueue queue;

  queue.set_max_size(1);

  EXPECT_EQ(queue.max_size(), std::size_t{1});
  EXPECT_TRUE(queue.bounded());

  EXPECT_TRUE(queue.push(make_task(1)));
  EXPECT_TRUE(queue.full());
  EXPECT_FALSE(queue.push(make_task(2)));
}
