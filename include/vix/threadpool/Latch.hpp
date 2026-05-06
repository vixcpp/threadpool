/**
 *
 * @file Latch.hpp
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
#ifndef VIX_THREADPOOL_LATCH_HPP
#define VIX_THREADPOOL_LATCH_HPP

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace vix::threadpool
{
  /**
   * @brief One-shot synchronization primitive.
   *
   * Latch starts with an initial counter. Threads may wait until the counter
   * reaches zero. Other threads call count_down() to decrement the counter.
   *
   * Unlike Barrier, Latch is one-shot:
   * - once it reaches zero, it stays open
   * - wait() returns immediately after that
   * - the counter cannot be reset
   *
   * This class is useful for waiting until a fixed number of tasks have reached
   * a completion point.
   */
  class Latch
  {
  public:
    /**
     * @brief Construct a latch with an initial counter.
     *
     * @param count Initial counter value.
     */
    explicit Latch(std::size_t count) noexcept
        : count_(count),
          mutex_(),
          cv_()
    {
    }

    Latch(const Latch &) = delete;
    Latch &operator=(const Latch &) = delete;

    Latch(Latch &&) = delete;
    Latch &operator=(Latch &&) = delete;

    /**
     * @brief Destroy the latch.
     */
    ~Latch() = default;

    /**
     * @brief Decrement the counter by one.
     *
     * If the counter reaches zero, all waiting threads are notified.
     * Calling this function after the counter is already zero has no effect.
     */
    void count_down()
    {
      count_down(1);
    }

    /**
     * @brief Decrement the counter by a given amount.
     *
     * If the amount is greater than the current counter, the counter becomes
     * zero. If the counter reaches zero, all waiting threads are notified.
     *
     * @param amount Amount to subtract.
     */
    void count_down(std::size_t amount)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (count_ == 0)
        {
          return;
        }

        if (amount >= count_)
        {
          count_ = 0;
        }
        else
        {
          count_ -= amount;
        }
      }

      cv_.notify_all();
    }

    /**
     * @brief Decrement the counter by one and wait until it reaches zero.
     */
    void arrive_and_wait()
    {
      count_down();
      wait();
    }

    /**
     * @brief Wait until the counter reaches zero.
     */
    void wait() const
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]()
               { return count_ == 0; });
    }

    /**
     * @brief Check whether the latch has reached zero.
     *
     * @return true if the latch is open.
     */
    [[nodiscard]] bool ready() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return count_ == 0;
    }

    /**
     * @brief Check whether the latch has reached zero.
     *
     * @return true if the latch is open.
     */
    [[nodiscard]] bool is_ready() const
    {
      return ready();
    }

    /**
     * @brief Return the current counter value.
     *
     * @return Current count.
     */
    [[nodiscard]] std::size_t count() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return count_;
    }

  private:
    /**
     * @brief Remaining counter value.
     */
    std::size_t count_;

    /**
     * @brief Mutex protecting the counter.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable used to wake waiters.
     */
    mutable std::condition_variable cv_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_LATCH_HPP
