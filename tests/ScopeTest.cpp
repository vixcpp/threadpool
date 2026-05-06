/**
 *
 * @file ScopeTest.cpp
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
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

#include <vix/threadpool/Scope.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>

TEST(ScopeTest, NewScopeIsOpenAndEmpty)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  EXPECT_TRUE(scope.empty());
  EXPECT_EQ(scope.size(), std::size_t{0});
  EXPECT_FALSE(scope.closed());
  EXPECT_FALSE(scope.cancelled());

  pool.shutdown();
}

TEST(ScopeTest, SpawnSubmitsAndTracksTask)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  std::atomic<int> counter{0};

  const bool accepted =
      scope.spawn(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  EXPECT_TRUE(accepted);
  EXPECT_FALSE(scope.empty());
  EXPECT_EQ(scope.size(), std::size_t{1});

  scope.wait();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
  EXPECT_TRUE(scope.empty());
  EXPECT_TRUE(scope.closed());

  pool.shutdown();
}

TEST(ScopeTest, SpawnRejectsAfterClose)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  scope.close();

  EXPECT_TRUE(scope.closed());

  const bool accepted =
      scope.spawn(
          []() {});

  EXPECT_FALSE(accepted);
  EXPECT_TRUE(scope.empty());

  pool.shutdown();
}

TEST(ScopeTest, WaitWaitsForAllSpawnedTasks)
{
  vix::threadpool::ThreadPool pool(4);
  vix::threadpool::Scope scope(pool);

  std::atomic<int> counter{0};

  for (int i = 0; i < 8; ++i)
  {
    EXPECT_TRUE(
        scope.spawn(
            [&counter]()
            {
              std::this_thread::sleep_for(std::chrono::milliseconds{5});
              counter.fetch_add(1, std::memory_order_relaxed);
            }));
  }

  scope.wait();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 8);
  EXPECT_TRUE(scope.empty());
  EXPECT_TRUE(scope.closed());

  pool.shutdown();
}

TEST(ScopeTest, WaitSwallowsTaskExceptions)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  EXPECT_TRUE(
      scope.spawn(
          []()
          {
            throw std::runtime_error{"scope failure"};
          }));

  EXPECT_NO_THROW(scope.wait());

  pool.shutdown();
}

TEST(ScopeTest, WaitAndRethrowRethrowsFirstTaskException)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  EXPECT_TRUE(
      scope.spawn(
          []()
          {
            throw std::runtime_error{"scope failure"};
          }));

  EXPECT_THROW(scope.wait_and_rethrow(), std::runtime_error);

  pool.shutdown();
}

TEST(ScopeTest, WaitAndRethrowWaitsForAllTasksBeforeThrowing)
{
  vix::threadpool::ThreadPool pool(4);
  vix::threadpool::Scope scope(pool);

  std::atomic<int> completed{0};

  EXPECT_TRUE(
      scope.spawn(
          []()
          {
            throw std::runtime_error{"first failure"};
          }));

  for (int i = 0; i < 4; ++i)
  {
    EXPECT_TRUE(
        scope.spawn(
            [&completed]()
            {
              std::this_thread::sleep_for(std::chrono::milliseconds{5});
              completed.fetch_add(1, std::memory_order_relaxed);
            }));
  }

  EXPECT_THROW(scope.wait_and_rethrow(), std::runtime_error);
  EXPECT_EQ(completed.load(std::memory_order_relaxed), 4);

  pool.shutdown();
}

TEST(ScopeTest, CancelRequestsSharedCancellation)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  const vix::threadpool::CancellationToken token =
      scope.cancellation_token();

  EXPECT_TRUE(token.can_cancel());
  EXPECT_FALSE(token.cancelled());

  scope.cancel();

  EXPECT_TRUE(scope.cancelled());
  EXPECT_TRUE(token.cancelled());

  pool.shutdown();
}

TEST(ScopeTest, SpawnedTaskReceivesScopeCancellationToken)
{
  vix::threadpool::ThreadPool pool(1);
  vix::threadpool::Scope scope(pool);

  scope.cancel();

  std::atomic<int> counter{0};

  const bool accepted =
      scope.spawn(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  EXPECT_TRUE(accepted);

  scope.wait();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 0);

  pool.shutdown();
}

TEST(ScopeTest, DestructorWaitsAndSwallowsExceptions)
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> counter{0};

  {
    vix::threadpool::Scope scope(pool);

    EXPECT_TRUE(
        scope.spawn(
            [&counter]()
            {
              std::this_thread::sleep_for(std::chrono::milliseconds{5});
              counter.fetch_add(1, std::memory_order_relaxed);
            }));

    EXPECT_TRUE(
        scope.spawn(
            []()
            {
              throw std::runtime_error{"ignored"};
            }));
  }

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);

  pool.shutdown();
}

TEST(ScopeTest, SpawnAcceptsTaskOptions)
{
  vix::threadpool::ThreadPool pool(2);
  vix::threadpool::Scope scope(pool);

  std::atomic<int> counter{0};

  vix::threadpool::TaskOptions options;
  options.set_priority(vix::threadpool::TaskPriority::high);

  EXPECT_TRUE(
      scope.spawn(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          },
          options));

  scope.wait();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);

  pool.shutdown();
}
