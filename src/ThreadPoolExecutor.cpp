/**
 *
 * @file ThreadPoolExecutor.cpp
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
#include <vix/threadpool/ThreadPoolExecutor.hpp>

#include <vix/threadpool/ThreadPool.hpp>

#include <utility>

namespace vix::threadpool
{
  ThreadPoolExecutor::ThreadPoolExecutor() noexcept
      : pool_(nullptr)
  {
  }

  ThreadPoolExecutor::ThreadPoolExecutor(ThreadPool &pool) noexcept
      : pool_(&pool)
  {
  }

  ThreadPoolExecutor::ThreadPoolExecutor(ThreadPoolExecutor &&other) noexcept
      : pool_(other.pool_)
  {
    other.pool_ = nullptr;
  }

  ThreadPoolExecutor &ThreadPoolExecutor::operator=(
      ThreadPoolExecutor &&other) noexcept
  {
    if (this == &other)
    {
      return *this;
    }

    pool_ = other.pool_;
    other.pool_ = nullptr;

    return *this;
  }

  ThreadPoolExecutor::~ThreadPoolExecutor() = default;

  void ThreadPoolExecutor::reset(ThreadPool &pool) noexcept
  {
    pool_ = &pool;
  }

  void ThreadPoolExecutor::reset() noexcept
  {
    pool_ = nullptr;
  }

  bool ThreadPoolExecutor::valid() const noexcept
  {
    return pool_ != nullptr;
  }

  ThreadPoolExecutor::operator bool() const noexcept
  {
    return valid();
  }

  bool ThreadPoolExecutor::post(
      Task task,
      TaskOptions options)
  {
    if (pool_ == nullptr)
    {
      return false;
    }

    return pool_->post(std::move(task), std::move(options));
  }

  void ThreadPoolExecutor::shutdown() noexcept
  {
    if (pool_ != nullptr)
    {
      pool_->shutdown();
    }
  }

  void ThreadPoolExecutor::wait_idle()
  {
    if (pool_ != nullptr)
    {
      pool_->wait_idle();
    }
  }

  bool ThreadPoolExecutor::running() const noexcept
  {
    return pool_ != nullptr && pool_->running();
  }

  bool ThreadPoolExecutor::idle() const
  {
    return pool_ == nullptr || pool_->idle();
  }

  ThreadPoolMetrics ThreadPoolExecutor::metrics() const
  {
    if (pool_ == nullptr)
    {
      return ThreadPoolMetrics{};
    }

    return pool_->metrics();
  }

  ThreadPoolStats ThreadPoolExecutor::stats() const
  {
    if (pool_ == nullptr)
    {
      return ThreadPoolStats{};
    }

    return pool_->stats();
  }

  ThreadPool *ThreadPoolExecutor::pool() noexcept
  {
    return pool_;
  }

  const ThreadPool *ThreadPoolExecutor::pool() const noexcept
  {
    return pool_;
  }

} // namespace vix::threadpool
