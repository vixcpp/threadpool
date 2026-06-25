/**
 *
 * @file ParallelReduceTest.cpp
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
#include <list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/ParallelReduce.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/ThreadPool.hpp>

TEST(ParallelReduceTest, EmptyRangeReturnsInitialValue)
{
  vix::threadpool::ThreadPool pool(2);

  std::vector<int> values;

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values.begin(),
          values.end(),
          42,
          [](int current, int value)
          {
            return current + value;
          });

  EXPECT_EQ(result, 42);

  pool.shutdown();
}

TEST(ParallelReduceTest, ReducesVectorElements)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(100);
  std::iota(values.begin(), values.end(), 1);

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            return current + value;
          });

  EXPECT_EQ(result, 5050);

  pool.shutdown();
}

TEST(ParallelReduceTest, ReducesIteratorRange)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values.begin(),
          values.end(),
          0,
          [](int current, int value)
          {
            return current + value;
          });

  EXPECT_EQ(result, 10);

  pool.shutdown();
}

TEST(ParallelReduceTest, SupportsListIterators)
{
  vix::threadpool::ThreadPool pool(4);

  std::list<int> values{1, 2, 3, 4, 5};

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values.begin(),
          values.end(),
          0,
          [](int current, int value)
          {
            return current + value;
          },
          vix::threadpool::ParallelReduceOptions::with_chunk_size(2));

  EXPECT_EQ(result, 15);

  pool.shutdown();
}

TEST(ParallelReduceTest, SupportsStringAccumulation)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<std::string> values{"a", "b", "c"};

  const std::string result =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          std::string{},
          [](std::string current, const std::string &value)
          {
            return current + value;
          },
          vix::threadpool::ParallelReduceOptions::with_chunk_size(1));

  EXPECT_EQ(result.size(), std::size_t{3});
  EXPECT_NE(result.find('a'), std::string::npos);
  EXPECT_NE(result.find('b'), std::string::npos);
  EXPECT_NE(result.find('c'), std::string::npos);

  pool.shutdown();
}

TEST(ParallelReduceTest, UsesCustomChunkSize)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 1, 1, 1, 1, 1};

  vix::threadpool::ParallelReduceOptions options;
  options.chunk_size = 2;

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            return current + value;
          },
          options);

  EXPECT_EQ(result, 6);

  pool.shutdown();
}

TEST(ParallelReduceTest, OptionsFactorySetsChunkSize)
{
  const vix::threadpool::ParallelReduceOptions options =
      vix::threadpool::ParallelReduceOptions::with_chunk_size(8);

  EXPECT_EQ(options.chunk_size, std::size_t{8});
}

TEST(ParallelReduceTest, PropagatesFirstExceptionAfterWaitingForChunks)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4, 5};

  EXPECT_THROW(
      (void)vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            if (value == 4)
            {
              throw std::runtime_error{"parallel_reduce failed"};
            }

            return current + value;
          },
          vix::threadpool::ParallelReduceOptions::with_chunk_size(1)),
      std::runtime_error);

  pool.shutdown();
}

TEST(ParallelReduceTest, TemporaryPoolIteratorOverloadWorks)
{
  std::vector<int> values{1, 2, 3};

  const int result =
      vix::threadpool::parallel_reduce(
          values.begin(),
          values.end(),
          0,
          [](int current, int value)
          {
            return current + value;
          });

  EXPECT_EQ(result, 6);
}

TEST(ParallelReduceTest, TemporaryPoolContainerOverloadWorks)
{
  std::vector<int> values{2, 3, 4};

  const int result =
      vix::threadpool::parallel_reduce(
          values,
          1,
          [](int current, int value)
          {
            return current * value;
          });

  EXPECT_EQ(result, 24);
}

TEST(ParallelReduceTest, TaskOptionsAreAccepted)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  vix::threadpool::ParallelReduceOptions options;
  options.chunk_size = 2;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  const int result =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            return current + value;
          },
          options);

  EXPECT_EQ(result, 10);

  pool.shutdown();
}
