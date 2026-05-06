/**
 *
 * @file ParallelForTest.cpp
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
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/ThreadPool.hpp>

TEST(ParallelForTest, EmptyRangeDoesNothing)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  vix::threadpool::parallel_for(
      pool,
      10,
      10,
      [&counter](int)
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      });

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 0);

  pool.shutdown();
}

TEST(ParallelForTest, ReversedRangeDoesNothing)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  vix::threadpool::parallel_for(
      pool,
      10,
      5,
      [&counter](int)
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      });

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 0);

  pool.shutdown();
}

TEST(ParallelForTest, ProcessesEachIndex)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(100, 0);

  vix::threadpool::parallel_for(
      pool,
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = static_cast<int>(index + 1);
      });

  for (std::size_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(values[i], static_cast<int>(i + 1));
  }

  pool.shutdown();
}

TEST(ParallelForTest, UsesCustomChunkSize)
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  vix::threadpool::ParallelForOptions options;
  options.chunk_size = 3;

  vix::threadpool::parallel_for(
      pool,
      0,
      30,
      [&counter](int)
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      },
      options);

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 30);

  pool.shutdown();
}

TEST(ParallelForTest, OptionsFactorySetsChunkSize)
{
  const vix::threadpool::ParallelForOptions options =
      vix::threadpool::ParallelForOptions::with_chunk_size(8);

  EXPECT_EQ(options.chunk_size, std::size_t{8});
}

TEST(ParallelForTest, ComputeChunkSizeUsesRequestedValue)
{
  const std::size_t chunk =
      vix::threadpool::compute_parallel_chunk_size(
          std::size_t{100},
          std::size_t{4},
          std::size_t{7});

  EXPECT_EQ(chunk, std::size_t{7});
}

TEST(ParallelForTest, ComputeChunkSizeNeverReturnsZero)
{
  EXPECT_EQ(
      vix::threadpool::compute_parallel_chunk_size(
          std::size_t{0},
          std::size_t{0},
          std::size_t{0}),
      std::size_t{1});

  EXPECT_GE(
      vix::threadpool::compute_parallel_chunk_size(
          std::size_t{100},
          std::size_t{0},
          std::size_t{0}),
      std::size_t{1});
}

TEST(ParallelForTest, PropagatesFirstExceptionAfterWaitingForAllChunks)
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> visited{0};

  EXPECT_THROW(
      vix::threadpool::parallel_for(
          pool,
          0,
          20,
          [&visited](int index)
          {
            visited.fetch_add(1, std::memory_order_relaxed);

            if (index == 5)
            {
              throw std::runtime_error{"parallel_for failed"};
            }
          },
          vix::threadpool::ParallelForOptions::with_chunk_size(1)),
      std::runtime_error);

  EXPECT_GE(visited.load(std::memory_order_relaxed), 1);

  pool.shutdown();
}

TEST(ParallelForTest, TemporaryPoolOverloadWorks)
{
  std::vector<int> values(16, 0);

  vix::threadpool::parallel_for(
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = 1;
      });

  for (const int value : values)
  {
    EXPECT_EQ(value, 1);
  }
}

TEST(ParallelForTest, TaskOptionsAreAccepted)
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  vix::threadpool::ParallelForOptions options;
  options.chunk_size = 2;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  vix::threadpool::parallel_for(
      pool,
      0,
      12,
      [&counter](int)
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      },
      options);

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 12);

  pool.shutdown();
}
