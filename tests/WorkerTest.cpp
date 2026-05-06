/**
 *
 * @file WorkerTest.cpp
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
#include <optional>
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskId.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/Worker.hpp>
#include <vix/threadpool/WorkerId.hpp>
#include <vix/threadpool/WorkerState.hpp>

namespace
{
  vix::threadpool::Task make_worker_task(
      vix::threadpool::TaskId id,
      std::atomic<int> *counter = nullptr,
      vix::threadpool::TaskPriority priority =
          vix::threadpool::TaskPriority::normal,
      std::uint64_t sequence = 0)
  {
    vix::threadpool::TaskOptions options;
    options.set_priority(priority);

    return vix::threadpool::Task(
        id,
        vix::threadpool::TaskFunction(
            [counter]()
            {
              if (counter != nullptr)
              {
                counter->fetch_add(1, std::memory_order_relaxed);
              }
            }),
        options,
        sequence);
  }

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

TEST(WorkerTest, WorkerStartsWithExpectedIdentity)
{
  const vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0},
      std::size_t{8},
      "vix-test");

  EXPECT_EQ(worker.id(), vix::threadpool::WorkerId{1});
  EXPECT_EQ(worker.index(), std::size_t{0});
  EXPECT_EQ(worker.max_queue_size(), std::size_t{8});
  EXPECT_EQ(worker.name(), "vix-test");
  EXPECT_TRUE(worker.empty());
  EXPECT_FALSE(worker.full());
  EXPECT_FALSE(worker.joinable());
}

TEST(WorkerTest, SubmitRejectsInvalidTask)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  EXPECT_FALSE(worker.submit(vix::threadpool::Task{}));
  EXPECT_EQ(worker.rejected_tasks(), std::uint64_t{1});
  EXPECT_TRUE(worker.empty());
}

TEST(WorkerTest, SubmitAcceptsValidTask)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{1})));
  EXPECT_EQ(worker.accepted_tasks(), std::uint64_t{1});
  EXPECT_EQ(worker.size(), std::size_t{1});
}

TEST(WorkerTest, SubmitRejectsWhenQueueIsFull)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0},
      std::size_t{1});

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{1})));
  EXPECT_TRUE(worker.full());

  EXPECT_FALSE(worker.submit(make_worker_task(vix::threadpool::TaskId{2})));
  EXPECT_EQ(worker.rejected_tasks(), std::uint64_t{1});
  EXPECT_EQ(worker.size(), std::size_t{1});
}

TEST(WorkerTest, TryPopReturnsSubmittedTask)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{42})));

  std::optional<vix::threadpool::Task> task = worker.try_pop();

  ASSERT_TRUE(task.has_value());
  EXPECT_EQ(task->id(), vix::threadpool::TaskId{42});
  EXPECT_TRUE(worker.empty());
}

TEST(WorkerTest, ClearRemovesQueuedTasks)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{1})));
  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{2})));

  const std::size_t removed = worker.clear();

  EXPECT_EQ(removed, std::size_t{2});
  EXPECT_TRUE(worker.empty());
}

TEST(WorkerTest, StartRunsSubmittedTasks)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  std::atomic<int> counter{0};

  EXPECT_TRUE(worker.submit(make_worker_task(
      vix::threadpool::TaskId{1},
      &counter)));

  ASSERT_TRUE(worker.start());

  EXPECT_TRUE(
      wait_until_true(
          [&counter]()
          {
            return counter.load(std::memory_order_relaxed) == 1;
          }));

  worker.stop();
  worker.join();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
  EXPECT_GE(worker.executed_tasks(), std::uint64_t{1});
  EXPECT_GE(worker.completed_tasks(), std::uint64_t{1});
}

TEST(WorkerTest, FinishCallbackIsCalledAfterTaskCompletion)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  std::atomic<bool> callbackCalled{false};
  std::atomic<vix::threadpool::TaskId> finishedId{
      vix::threadpool::invalid_task_id};

  worker.set_finish_callback(
      [&callbackCalled, &finishedId](const vix::threadpool::Task &task)
      {
        finishedId.store(task.id(), std::memory_order_relaxed);
        callbackCalled.store(true, std::memory_order_relaxed);
      });

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{9})));

  ASSERT_TRUE(worker.start());

  EXPECT_TRUE(
      wait_until_true(
          [&callbackCalled]()
          {
            return callbackCalled.load(std::memory_order_relaxed);
          }));

  worker.stop();
  worker.join();

  EXPECT_EQ(finishedId.load(std::memory_order_relaxed),
            vix::threadpool::TaskId{9});
}

TEST(WorkerTest, StopWithoutDrainCanLeaveQueuedTasks)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{1},
      std::size_t{0});

  worker.set_drain_on_stop(false);

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{1})));
  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{2})));

  worker.stop();

  EXPECT_TRUE(worker.stopping());
  EXPECT_FALSE(worker.submit(make_worker_task(vix::threadpool::TaskId{3})));

  worker.join();
}

TEST(WorkerTest, MetricsReflectWorkerCounters)
{
  vix::threadpool::Worker worker(
      vix::threadpool::WorkerId{3},
      std::size_t{2},
      std::size_t{4},
      "vix-worker");

  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{1})));
  EXPECT_TRUE(worker.submit(make_worker_task(vix::threadpool::TaskId{2})));

  const vix::threadpool::WorkerMetrics metrics = worker.metrics();

  EXPECT_EQ(metrics.id, vix::threadpool::WorkerId{3});
  EXPECT_EQ(metrics.index, std::size_t{2});
  EXPECT_EQ(metrics.pending_tasks, std::size_t{2});
  EXPECT_EQ(metrics.accepted_tasks, std::uint64_t{2});
  EXPECT_EQ(metrics.rejected_tasks, std::uint64_t{0});
}
