/**
 *
 * @file ThreadPoolMetricsTest.cpp
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
#include <cstdint>
#include <cstddef>

#include <gtest/gtest.h>

#include <vix/threadpool/ThreadPoolMetrics.hpp>

TEST(ThreadPoolMetricsTest, DefaultMetricsAreEmpty)
{
  const vix::threadpool::ThreadPoolMetrics metrics;

  EXPECT_EQ(metrics.worker_count, std::size_t{0});
  EXPECT_EQ(metrics.pending_tasks, std::size_t{0});
  EXPECT_EQ(metrics.active_tasks, std::uint64_t{0});
  EXPECT_EQ(metrics.idle_workers, std::size_t{0});
  EXPECT_EQ(metrics.busy_workers, std::size_t{0});
  EXPECT_EQ(metrics.submitted_tasks, std::uint64_t{0});
  EXPECT_EQ(metrics.completed_tasks, std::uint64_t{0});
  EXPECT_EQ(metrics.failed_tasks, std::uint64_t{0});
  EXPECT_EQ(metrics.cancelled_tasks, std::uint64_t{0});
  EXPECT_EQ(metrics.timed_out_tasks, std::uint64_t{0});
  EXPECT_EQ(metrics.rejected_tasks, std::uint64_t{0});

  EXPECT_TRUE(metrics.idle());
  EXPECT_EQ(metrics.finished_tasks(), std::uint64_t{0});
  EXPECT_EQ(metrics.error_tasks(), std::uint64_t{0});
}

TEST(ThreadPoolMetricsTest, IdleIsFalseWhenTasksArePending)
{
  vix::threadpool::ThreadPoolMetrics metrics;
  metrics.pending_tasks = 1;

  EXPECT_FALSE(metrics.idle());
}

TEST(ThreadPoolMetricsTest, IdleIsFalseWhenTasksAreActive)
{
  vix::threadpool::ThreadPoolMetrics metrics;
  metrics.active_tasks = 1;

  EXPECT_FALSE(metrics.idle());
}

TEST(ThreadPoolMetricsTest, FinishedTasksExcludesRejectedTasks)
{
  vix::threadpool::ThreadPoolMetrics metrics;
  metrics.completed_tasks = 10;
  metrics.failed_tasks = 2;
  metrics.cancelled_tasks = 3;
  metrics.timed_out_tasks = 4;
  metrics.rejected_tasks = 5;

  EXPECT_EQ(metrics.finished_tasks(), std::uint64_t{19});
}

TEST(ThreadPoolMetricsTest, ErrorTasksIncludesRejectedTasks)
{
  vix::threadpool::ThreadPoolMetrics metrics;
  metrics.completed_tasks = 10;
  metrics.failed_tasks = 2;
  metrics.cancelled_tasks = 3;
  metrics.timed_out_tasks = 4;
  metrics.rejected_tasks = 5;

  EXPECT_EQ(metrics.error_tasks(), std::uint64_t{14});
}

TEST(ThreadPoolMetricsTest, MetricsSnapshotCanRepresentWorkerState)
{
  vix::threadpool::ThreadPoolMetrics metrics;

  metrics.worker_count = 4;
  metrics.idle_workers = 3;
  metrics.busy_workers = 1;
  metrics.pending_tasks = 6;
  metrics.active_tasks = 1;
  metrics.submitted_tasks = 20;
  metrics.completed_tasks = 13;
  metrics.failed_tasks = 1;
  metrics.cancelled_tasks = 2;
  metrics.timed_out_tasks = 1;
  metrics.rejected_tasks = 3;

  EXPECT_EQ(metrics.worker_count, std::size_t{4});
  EXPECT_EQ(metrics.idle_workers, std::size_t{3});
  EXPECT_EQ(metrics.busy_workers, std::size_t{1});
  EXPECT_EQ(metrics.pending_tasks, std::size_t{6});
  EXPECT_EQ(metrics.active_tasks, std::uint64_t{1});
  EXPECT_EQ(metrics.submitted_tasks, std::uint64_t{20});
  EXPECT_EQ(metrics.finished_tasks(), std::uint64_t{17});
  EXPECT_EQ(metrics.error_tasks(), std::uint64_t{7});
  EXPECT_FALSE(metrics.idle());
}
