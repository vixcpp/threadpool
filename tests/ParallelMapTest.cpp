/**
 *
 * @file ParallelMapTest.cpp
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
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <vix/threadpool/ParallelMap.hpp>
#include <vix/threadpool/TaskPriority.hpp>
#include <vix/threadpool/ThreadPool.hpp>

TEST(ParallelMapTest, EmptyRangeReturnsEmptyVector)
{
  vix::threadpool::ThreadPool pool(2);

  std::vector<int> values;

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          pool,
          values.begin(),
          values.end(),
          [](int value)
          {
            return value * 2;
          });

  EXPECT_TRUE(result.empty());

  pool.shutdown();
}

TEST(ParallelMapTest, MapsVectorElementsInOrder)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4, 5};

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            return value * value;
          });

  ASSERT_EQ(result.size(), values.size());
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 4);
  EXPECT_EQ(result[2], 9);
  EXPECT_EQ(result[3], 16);
  EXPECT_EQ(result[4], 25);

  pool.shutdown();
}

TEST(ParallelMapTest, MapsIteratorRangeInOrder)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{10, 20, 30};

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          pool,
          values.begin(),
          values.end(),
          [](int value)
          {
            return value + 1;
          });

  ASSERT_EQ(result.size(), std::size_t{3});
  EXPECT_EQ(result[0], 11);
  EXPECT_EQ(result[1], 21);
  EXPECT_EQ(result[2], 31);

  pool.shutdown();
}

TEST(ParallelMapTest, SupportsDifferentResultType)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3};

  const std::vector<std::string> result =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            return std::string{"value-"} + std::to_string(value);
          });

  ASSERT_EQ(result.size(), std::size_t{3});
  EXPECT_EQ(result[0], "value-1");
  EXPECT_EQ(result[1], "value-2");
  EXPECT_EQ(result[2], "value-3");

  pool.shutdown();
}

TEST(ParallelMapTest, SupportsListIterators)
{
  vix::threadpool::ThreadPool pool(4);

  std::list<int> values{1, 2, 3, 4};

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          pool,
          values.begin(),
          values.end(),
          [](int value)
          {
            return value * 10;
          },
          vix::threadpool::ParallelMapOptions::with_chunk_size(2));

  ASSERT_EQ(result.size(), std::size_t{4});
  EXPECT_EQ(result[0], 10);
  EXPECT_EQ(result[1], 20);
  EXPECT_EQ(result[2], 30);
  EXPECT_EQ(result[3], 40);

  pool.shutdown();
}

TEST(ParallelMapTest, UsesCustomChunkSize)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4, 5, 6};

  vix::threadpool::ParallelMapOptions options;
  options.chunk_size = 2;

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            return value + 100;
          },
          options);

  ASSERT_EQ(result.size(), values.size());

  for (std::size_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(result[i], values[i] + 100);
  }

  pool.shutdown();
}

TEST(ParallelMapTest, OptionsFactorySetsChunkSize)
{
  const vix::threadpool::ParallelMapOptions options =
      vix::threadpool::ParallelMapOptions::with_chunk_size(8);

  EXPECT_EQ(options.chunk_size, std::size_t{8});
}

TEST(ParallelMapTest, PropagatesFirstExceptionAfterWaitingForChunks)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4, 5};

  EXPECT_THROW(
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            if (value == 3)
            {
              throw std::runtime_error{"parallel_map failed"};
            }

            return value;
          },
          vix::threadpool::ParallelMapOptions::with_chunk_size(1)),
      std::runtime_error);

  pool.shutdown();
}

TEST(ParallelMapTest, TemporaryPoolIteratorOverloadWorks)
{
  std::vector<int> values{1, 2, 3};

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          values.begin(),
          values.end(),
          [](int value)
          {
            return value * 3;
          });

  ASSERT_EQ(result.size(), std::size_t{3});
  EXPECT_EQ(result[0], 3);
  EXPECT_EQ(result[1], 6);
  EXPECT_EQ(result[2], 9);
}

TEST(ParallelMapTest, TemporaryPoolContainerOverloadWorks)
{
  std::vector<std::string> values{"a", "bb", "ccc"};

  const std::vector<std::size_t> result =
      vix::threadpool::parallel_map(
          values,
          [](const std::string &value)
          {
            return value.size();
          });

  ASSERT_EQ(result.size(), std::size_t{3});
  EXPECT_EQ(result[0], std::size_t{1});
  EXPECT_EQ(result[1], std::size_t{2});
  EXPECT_EQ(result[2], std::size_t{3});
}

TEST(ParallelMapTest, TaskOptionsAreAccepted)
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  vix::threadpool::ParallelMapOptions options;
  options.chunk_size = 2;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  const std::vector<int> result =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            return value + 1;
          },
          options);

  ASSERT_EQ(result.size(), values.size());
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 3);
  EXPECT_EQ(result[2], 4);
  EXPECT_EQ(result[3], 5);

  pool.shutdown();
}
