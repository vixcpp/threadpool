/**
 *
 * @file TaskGuard.hpp
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
#ifndef VIX_THREADPOOL_TASK_GUARD_HPP
#define VIX_THREADPOOL_TASK_GUARD_HPP

#include <atomic>
#include <type_traits>

namespace vix::threadpool
{
  /**
   * @brief RAII guard used to track active task counters.
   *
   * TaskGuard increments a shared atomic counter on construction and decrements
   * it on destruction. It is used by worker code to keep active task metrics
   * correct even when task execution exits early.
   *
   * @tparam T Integral counter type.
   */
  template <class T>
  class TaskGuard
  {
    static_assert(std::is_integral_v<T>,
                  "TaskGuard<T> requires an integral counter type");

  public:
    /**
     * @brief Counter value type.
     */
    using value_type = T;

    /**
     * @brief Construct a task guard and increment the counter.
     *
     * @param counter Atomic counter to update.
     */
    explicit TaskGuard(std::atomic<T> &counter) noexcept
        : counter_(&counter),
          active_(true)
    {
      counter_->fetch_add(1, std::memory_order_relaxed);
    }

    TaskGuard(const TaskGuard &) = delete;
    TaskGuard &operator=(const TaskGuard &) = delete;

    /**
     * @brief Move-construct a task guard.
     *
     * The moved-from guard is released so only one guard decrements the counter.
     *
     * @param other Source guard.
     */
    TaskGuard(TaskGuard &&other) noexcept
        : counter_(other.counter_),
          active_(other.active_)
    {
      other.counter_ = nullptr;
      other.active_ = false;
    }

    TaskGuard &operator=(TaskGuard &&) = delete;

    /**
     * @brief Decrement the counter if this guard is still active.
     */
    ~TaskGuard() noexcept
    {
      release();
    }

    /**
     * @brief Release this guard and decrement the counter once.
     *
     * Calling release() multiple times is safe.
     */
    void release() noexcept
    {
      if (!active_ || counter_ == nullptr)
      {
        return;
      }

      counter_->fetch_sub(1, std::memory_order_relaxed);
      active_ = false;
    }

    /**
     * @brief Check whether this guard still owns a counter increment.
     *
     * @return true if the guard will decrement the counter on destruction.
     */
    [[nodiscard]] bool active() const noexcept
    {
      return active_;
    }

  private:
    /**
     * @brief Counter updated by this guard.
     */
    std::atomic<T> *counter_;

    /**
     * @brief Whether this guard still owns the counter increment.
     */
    bool active_;
  };

  /**
   * @brief Deduction guide for TaskGuard.
   *
   * @tparam T Integral counter type.
   */
  template <class T>
  TaskGuard(std::atomic<T> &) -> TaskGuard<T>;

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_GUARD_HPP
