/**
 *
 * @file ThreadPoolConfigTest.cpp
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
#include <cstddef>

#include <gtest/gtest.h>

#include <vix/threadpool/ThreadPoolConfig.hpp>
#include <vix/threadpool/TaskPriority.hpp>

TEST(ThreadPoolConfigTest, DefaultConfigIsValid)
{
  const vix::threadpool::ThreadPoolConfig config;

  EXPECT_GE(config.thread_count, std::size_t{1});
  EXPECT_GE(config.max_thread_count, config.thread_count);
  EXPECT_EQ(config.max_queue_size, std::size_t{0});
  EXPECT_EQ(config.default_priority, vix::threadpool::TaskPriority::normal);
  EXPECT_FALSE(config.allow_dynamic_growth);
  EXPECT_TRUE(config.drain_on_shutdown);
  EXPECT_TRUE(config.swallow_task_exceptions);
  EXPECT_EQ(config.idle_wait, std::chrono::microseconds{0});
  EXPECT_EQ(config.default_timeout, std::chrono::milliseconds{0});
}

TEST(ThreadPoolConfigTest, DefaultThreadCountIsNeverZero)
{
  const std::size_t count =
      vix::threadpool::ThreadPoolConfig::default_thread_count();

  EXPECT_GE(count, std::size_t{1});
}

TEST(ThreadPoolConfigTest, NormalizeUsesDefaultThreadCountWhenThreadCountIsZero)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 0;
  config.max_thread_count = 0;

  const vix::threadpool::ThreadPoolConfig normalized = config.normalized();

  EXPECT_GE(normalized.thread_count, std::size_t{1});
  EXPECT_GE(normalized.max_thread_count, normalized.thread_count);
}

TEST(ThreadPoolConfigTest, NormalizeRaisesMaxThreadCountWhenTooSmall)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 4;
  config.max_thread_count = 2;

  const vix::threadpool::ThreadPoolConfig normalized = config.normalized();

  EXPECT_EQ(normalized.thread_count, std::size_t{4});
  EXPECT_EQ(normalized.max_thread_count, std::size_t{4});
}

TEST(ThreadPoolConfigTest, NormalizeKeepsExplicitMaxThreadCountWhenValid)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 2;
  config.max_thread_count = 8;

  const vix::threadpool::ThreadPoolConfig normalized = config.normalized();

  EXPECT_EQ(normalized.thread_count, std::size_t{2});
  EXPECT_EQ(normalized.max_thread_count, std::size_t{8});
}

TEST(ThreadPoolConfigTest, NormalizeKeepsQueueAndBehaviorOptions)
{
  vix::threadpool::ThreadPoolConfig config;
  config.thread_count = 2;
  config.max_thread_count = 4;
  config.max_queue_size = 128;
  config.default_priority = vix::threadpool::TaskPriority::high;
  config.allow_dynamic_growth = true;
  config.drain_on_shutdown = false;
  config.swallow_task_exceptions = false;
  config.idle_wait = std::chrono::microseconds{50};
  config.default_timeout = std::chrono::milliseconds{250};

  const vix::threadpool::ThreadPoolConfig normalized = config.normalized();

  EXPECT_EQ(normalized.thread_count, std::size_t{2});
  EXPECT_EQ(normalized.max_thread_count, std::size_t{4});
  EXPECT_EQ(normalized.max_queue_size, std::size_t{128});
  EXPECT_EQ(normalized.default_priority, vix::threadpool::TaskPriority::high);
  EXPECT_TRUE(normalized.allow_dynamic_growth);
  EXPECT_FALSE(normalized.drain_on_shutdown);
  EXPECT_FALSE(normalized.swallow_task_exceptions);
  EXPECT_EQ(normalized.idle_wait, std::chrono::microseconds{50});
  EXPECT_EQ(normalized.default_timeout, std::chrono::milliseconds{250});
}
