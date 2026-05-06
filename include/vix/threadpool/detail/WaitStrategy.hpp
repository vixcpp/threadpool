/**
 *
 * @file WaitStrategy.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_WAIT_STRATEGY_HPP
#define VIX_THREADPOOL_DETAIL_WAIT_STRATEGY_HPP

#include <chrono>
#include <cstdint>
#include <thread>

namespace vix::threadpool::detail
{
  /**
   * @brief Strategy used by idle workers when no task is immediately available.
   *
   * WaitStrategy keeps idle waiting behavior centralized so workers can avoid
   * permanent hot spinning while still staying responsive for short bursts of
   * work.
   */
  class WaitStrategy
  {
  public:
    /**
     * @brief Duration used by the pause phase.
     */
    using pause_duration = std::chrono::microseconds;

    /**
     * @brief Construct a default wait strategy.
     *
     * The default strategy:
     * - yields first
     * - then performs a very short sleep
     * - then performs a slightly longer sleep after sustained idleness
     */
    constexpr WaitStrategy() noexcept
        : yield_limit_(32),
          short_sleep_limit_(128),
          short_sleep_(pause_duration{2}),
          long_sleep_(pause_duration{10})
    {
    }

    /**
     * @brief Construct a custom wait strategy.
     *
     * @param yieldLimit Number of initial idle cycles using std::this_thread::yield().
     * @param shortSleepLimit Number of idle cycles using the short sleep phase.
     * @param shortSleep Short sleep duration.
     * @param longSleep Long sleep duration used after sustained idleness.
     */
    constexpr WaitStrategy(
        std::uint32_t yieldLimit,
        std::uint32_t shortSleepLimit,
        pause_duration shortSleep,
        pause_duration longSleep) noexcept
        : yield_limit_(yieldLimit),
          short_sleep_limit_(shortSleepLimit < yieldLimit ? yieldLimit : shortSleepLimit),
          short_sleep_(shortSleep),
          long_sleep_(longSleep)
    {
    }

    /**
     * @brief Wait according to the current idle streak.
     *
     * @param idleStreak Number of consecutive times the worker found no work.
     */
    void wait(std::uint32_t idleStreak) const noexcept
    {
      if (idleStreak <= yield_limit_)
      {
        std::this_thread::yield();
        return;
      }

      if (idleStreak <= short_sleep_limit_)
      {
        std::this_thread::sleep_for(short_sleep_);
        return;
      }

      std::this_thread::sleep_for(long_sleep_);
    }

    /**
     * @brief Return the configured yield phase limit.
     *
     * @return Number of idle cycles using yield.
     */
    [[nodiscard]] constexpr std::uint32_t yield_limit() const noexcept
    {
      return yield_limit_;
    }

    /**
     * @brief Return the configured short sleep phase limit.
     *
     * @return Number of idle cycles using short sleep.
     */
    [[nodiscard]] constexpr std::uint32_t short_sleep_limit() const noexcept
    {
      return short_sleep_limit_;
    }

    /**
     * @brief Return the configured short sleep duration.
     *
     * @return Short sleep duration.
     */
    [[nodiscard]] constexpr pause_duration short_sleep() const noexcept
    {
      return short_sleep_;
    }

    /**
     * @brief Return the configured long sleep duration.
     *
     * @return Long sleep duration.
     */
    [[nodiscard]] constexpr pause_duration long_sleep() const noexcept
    {
      return long_sleep_;
    }

  private:
    /**
     * @brief Maximum idle streak handled with std::this_thread::yield().
     */
    std::uint32_t yield_limit_;

    /**
     * @brief Maximum idle streak handled with short sleeps.
     */
    std::uint32_t short_sleep_limit_;

    /**
     * @brief Sleep duration used after the yield phase.
     */
    pause_duration short_sleep_;

    /**
     * @brief Sleep duration used after sustained idleness.
     */
    pause_duration long_sleep_;
  };

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_WAIT_STRATEGY_HPP
