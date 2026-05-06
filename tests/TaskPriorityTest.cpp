/**
 *
 * @file TaskPriorityTest.cpp
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

#include <gtest/gtest.h>

#include <vix/threadpool/TaskPriority.hpp>

TEST(TaskPriorityTest, PriorityValuesAreStable)
{
  EXPECT_EQ(
      vix::threadpool::to_priority_value(
          vix::threadpool::TaskPriority::lowest),
      std::int32_t{-2});

  EXPECT_EQ(
      vix::threadpool::to_priority_value(
          vix::threadpool::TaskPriority::low),
      std::int32_t{-1});

  EXPECT_EQ(
      vix::threadpool::to_priority_value(
          vix::threadpool::TaskPriority::normal),
      std::int32_t{0});

  EXPECT_EQ(
      vix::threadpool::to_priority_value(
          vix::threadpool::TaskPriority::high),
      std::int32_t{1});

  EXPECT_EQ(
      vix::threadpool::to_priority_value(
          vix::threadpool::TaskPriority::highest),
      std::int32_t{2});
}

TEST(TaskPriorityTest, HigherThanComparesPriorityValues)
{
  EXPECT_TRUE(
      vix::threadpool::priority_higher_than(
          vix::threadpool::TaskPriority::high,
          vix::threadpool::TaskPriority::normal));

  EXPECT_TRUE(
      vix::threadpool::priority_higher_than(
          vix::threadpool::TaskPriority::highest,
          vix::threadpool::TaskPriority::low));

  EXPECT_FALSE(
      vix::threadpool::priority_higher_than(
          vix::threadpool::TaskPriority::normal,
          vix::threadpool::TaskPriority::high));

  EXPECT_FALSE(
      vix::threadpool::priority_higher_than(
          vix::threadpool::TaskPriority::normal,
          vix::threadpool::TaskPriority::normal));
}

TEST(TaskPriorityTest, ToStringReturnsReadableNames)
{
  EXPECT_STREQ(
      vix::threadpool::to_string(
          vix::threadpool::TaskPriority::lowest),
      "lowest");

  EXPECT_STREQ(
      vix::threadpool::to_string(
          vix::threadpool::TaskPriority::low),
      "low");

  EXPECT_STREQ(
      vix::threadpool::to_string(
          vix::threadpool::TaskPriority::normal),
      "normal");

  EXPECT_STREQ(
      vix::threadpool::to_string(
          vix::threadpool::TaskPriority::high),
      "high");

  EXPECT_STREQ(
      vix::threadpool::to_string(
          vix::threadpool::TaskPriority::highest),
      "highest");
}

TEST(TaskPriorityTest, UnknownPriorityReturnsUnknown)
{
  const auto invalid =
      static_cast<vix::threadpool::TaskPriority>(123);

  EXPECT_STREQ(vix::threadpool::to_string(invalid), "unknown");
}
