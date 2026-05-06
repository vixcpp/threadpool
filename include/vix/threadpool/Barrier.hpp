/**
 *
 * @file Barrier.hpp
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
#ifndef VIX_THREADPOOL_BARRIER_HPP
#define VIX_THREADPOOL_BARRIER_HPP

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace vix::threadpool
{
  /**
   * @brief Reusable synchronization barrier.
   *
   * Barrier waits until a fixed number of participants arrive. Once the last
   * participant arrives, all waiting participants are released and the barrier
   * automatically resets for the next generation.
   *
   * This class is useful for phased parallel work where multiple tasks must
   * reach the same checkpoint before continuing.
   */
  class Barrier
  {
  public:
    /**
     * @brief Construct a barrier with a participant count.
     *
     * A participant count of zero is normalized to one.
     *
     * @param participants Number of participants required to release the barrier.
     */
    explicit Barrier(std::size_t participants) noexcept
        : initial_(participants == 0 ? 1 : participants),
          remaining_(participants == 0 ? 1 : participants),
          generation_(0),
          mutex_(),
          cv_()
    {
    }

    Barrier(const Barrier &) = delete;
    Barrier &operator=(const Barrier &) = delete;

    Barrier(Barrier &&) = delete;
    Barrier &operator=(Barrier &&) = delete;

    /**
     * @brief Destroy the barrier.
     */
    ~Barrier() = default;

    /**
     * @brief Arrive at the barrier and wait for all participants.
     *
     * The last arriving participant advances the generation, resets the
     * remaining counter, and wakes all waiters.
     */
    void arrive_and_wait()
    {
      std::unique_lock<std::mutex> lock(mutex_);

      const std::size_t currentGeneration = generation_;

      if (remaining_ > 0)
      {
        --remaining_;
      }

      if (remaining_ == 0)
      {
        remaining_ = initial_;
        ++generation_;
        lock.unlock();
        cv_.notify_all();
        return;
      }

      cv_.wait(lock, [this, currentGeneration]()
               { return generation_ != currentGeneration; });
    }

    /**
     * @brief Arrive at the barrier without waiting.
     *
     * If this arrival completes the current generation, all waiters are
     * released.
     */
    void arrive()
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        if (remaining_ > 0)
        {
          --remaining_;
        }

        if (remaining_ != 0)
        {
          return;
        }

        remaining_ = initial_;
        ++generation_;
      }

      cv_.notify_all();
    }

    /**
     * @brief Wait until the barrier advances to a new generation.
     *
     * This does not count as an arrival.
     */
    void wait()
    {
      std::unique_lock<std::mutex> lock(mutex_);

      const std::size_t currentGeneration = generation_;

      cv_.wait(lock, [this, currentGeneration]()
               { return generation_ != currentGeneration; });
    }

    /**
     * @brief Force the current generation to complete.
     *
     * This releases all current waiters and resets the barrier for the next
     * generation.
     */
    void release()
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        remaining_ = initial_;
        ++generation_;
      }

      cv_.notify_all();
    }

    /**
     * @brief Return the configured participant count.
     *
     * @return Number of participants required per generation.
     */
    [[nodiscard]] std::size_t participants() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return initial_;
    }

    /**
     * @brief Return how many participants are still missing in this generation.
     *
     * @return Remaining participant count.
     */
    [[nodiscard]] std::size_t remaining() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return remaining_;
    }

    /**
     * @brief Return the current barrier generation.
     *
     * The generation is incremented each time the barrier releases.
     *
     * @return Current generation number.
     */
    [[nodiscard]] std::size_t generation() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return generation_;
    }

  private:
    /**
     * @brief Number of participants required per generation.
     */
    std::size_t initial_;

    /**
     * @brief Number of participants still missing in the current generation.
     */
    std::size_t remaining_;

    /**
     * @brief Current barrier generation.
     */
    std::size_t generation_;

    /**
     * @brief Mutex protecting barrier state.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable used to wake barrier waiters.
     */
    std::condition_variable cv_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_BARRIER_HPP
