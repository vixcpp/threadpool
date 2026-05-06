/**
 *
 * @file ParallelForEachTest.cpp
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
#include <forward_list>
#include <list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/ParallelForEach.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/ThreadPool.hpp>

TEST(ParallelForEachTest, EmptyRangeDoesNothing)
{
  vix::threadpool::ThreadPool pool(2);

  std::vector<int> values;
  std::atomic<int> counter{0};

  vix::threadpool::parallel_for_each(
      pool,
      values.begin(),
      values.end(),
      [&counter](int)
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      });

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 0);

  pool.shutdown();
}

TEST(ParallelForEachTest, ProcessesVectorElements)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(32, 0);

  vix::threadpool::parallel_for_each(
      pool,
      values,
      [](int &value)
      {
        value = 7;
      });

  for (const int value : values)
  {
    EXPECT_EQ(value, 7);
  }

  pool.shutdown();
}

TEST(ParallelForEachTest, ProcessesIteratorRange)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4, 5};

  vix::threadpool::parallel_for_each(
      pool,
      values.begin(),
      values.end(),
      [](int &value)
      {
        value *= 2;
      });

  EXPECT_EQ(values[0], 2);
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 6);
  EXPECT_EQ(values[3], 8);
  EXPECT_EQ(values[4], 10);

  pool.shutdown();
}

TEST(ParallelForEachTest, SupportsListIterators)
{
  vix::threadpool::ThreadPool pool(4);

  std::list<int> values{1, 2, 3, 4, 5};

  vix::threadpool::parallel_for_each(
      pool,
      values.begin(),
      values.end(),
      [](int &value)
      {
        value += 10;
      },
      vix::threadpool::ParallelForEachOptions::with_chunk_size(2));

  for (const int value : values)
  {
    EXPECT_GE(value, 11);
    EXPECT_LE(value, 15);
  }

  pool.shutdown();
}

TEST(ParallelForEachTest, SupportsForwardListIterators)
{
  vix::threadpool::ThreadPool pool(4);

  std::forward_list<int> values{1, 2, 3, 4};

  vix::threadpool::parallel_for_each(
      pool,
      values.begin(),
      values.end(),
      [](int &value)
      {
        value = value * value;
      },
      vix::threadpool::ParallelForEachOptions::with_chunk_size(1));

  std::vector<int> output(values.begin(), values.end());

  ASSERT_EQ(output.size(), std::size_t{4});
  EXPECT_EQ(output[0], 1);
  EXPECT_EQ(output[1], 4);
  EXPECT_EQ(output[2], 9);
  EXPECT_EQ(output[3], 16);

  pool.shutdown();
}

TEST(ParallelForEachTest, UsesCustomChunkSize)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(25, 0);
  std::atomic<int> counter{0};

  vix::threadpool::ParallelForEachOptions options;
  options.chunk_size = 3;

  vix::threadpool::parallel_for_each(
      pool,
      values,
      [&counter](int &value)
      {
        value = 1;
        counter.fetch_add(1, std::memory_order_relaxed);
      },
      options);

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 25);

  for (const int value : values)
  {
    EXPECT_EQ(value, 1);
  }

  pool.shutdown();
}

TEST(ParallelForEachTest, OptionsFactorySetsChunkSize)
{
  const vix::threadpool::ParallelForEachOptions options =
      vix::threadpool::ParallelForEachOptions::with_chunk_size(9);

  EXPECT_EQ(options.chunk_size, std::size_t{9});
}

TEST(ParallelForEachTest, PropagatesFirstExceptionAfterWaitingForAllChunks)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(20, 0);
  std::atomic<int> visited{0};

  EXPECT_THROW(
      vix::threadpool::parallel_for_each(
          pool,
          values,
          [&visited](int &value)
          {
            visited.fetch_add(1, std::memory_order_relaxed);

            if (visited.load(std::memory_order_relaxed) == 5)
            {
              throw std::runtime_error{"parallel_for_each failed"};
            }

            value = 1;
          },
          vix::threadpool::ParallelForEachOptions::with_chunk_size(1)),
      std::runtime_error);

  EXPECT_GE(visited.load(std::memory_order_relaxed), 1);

  pool.shutdown();
}

TEST(ParallelForEachTest, TemporaryPoolIteratorOverloadWorks)
{
  std::vector<int> values(10, 0);

  vix::threadpool::parallel_for_each(
      values.begin(),
      values.end(),
      [](int &value)
      {
        value = 3;
      });

  for (const int value : values)
  {
    EXPECT_EQ(value, 3);
  }
}

TEST(ParallelForEachTest, TemporaryPoolContainerOverloadWorks)
{
  std::vector<std::string> values{"a", "b", "c"};

  vix::threadpool::parallel_for_each(
      values,
      [](std::string &value)
      {
        value += value;
      });

  EXPECT_EQ(values[0], "aa");
  EXPECT_EQ(values[1], "bb");
  EXPECT_EQ(values[2], "cc");
}

TEST(ParallelForEachTest, TaskOptionsAreAccepted)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(12, 0);

  vix::threadpool::ParallelForEachOptions options;
  options.chunk_size = 2;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  vix::threadpool::parallel_for_each(
      pool,
      values,
      [](int &value)
      {
        value = 5;
      },
      options);

  for (const int value : values)
  {
    EXPECT_EQ(value, 5);
  }

  pool.shutdown();
}
