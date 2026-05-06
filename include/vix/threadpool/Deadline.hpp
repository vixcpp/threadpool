/**
 *
 * @file Deadline.hpp
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
#ifndef VIX_THREADPOOL_DEADLINE_HPP
#define VIX_THREADPOOL_DEADLINE_HPP

#include <chrono>

#include <vix/threadpool/Timeout.hpp>

namespace vix::threadpool
{
  /**
   * @brief Absolute time limit for a task or operation.
   *
   * Deadline stores a steady_clock time point. It is used when an operation
   * should be considered expired after a specific point in time.
   *
   * A default-constructed Deadline is disabled and never expires.
   */
  class Deadline
  {
  public:
    /**
     * @brief Clock type used by deadlines.
     */
    using clock = std::chrono::steady_clock;

    /**
     * @brief Time point type used by deadlines.
     */
    using time_point = clock::time_point;

    /**
     * @brief Construct a disabled deadline.
     */
    constexpr Deadline() noexcept
        : time_(time_point{}),
          enabled_(false)
    {
    }

    /**
     * @brief Construct an enabled deadline from a time point.
     *
     * @param time Absolute deadline time.
     */
    explicit constexpr Deadline(time_point time) noexcept
        : time_(time),
          enabled_(true)
    {
    }

    /**
     * @brief Create a disabled deadline.
     *
     * @return Disabled deadline.
     */
    [[nodiscard]] static constexpr Deadline disabled() noexcept
    {
      return Deadline{};
    }

    /**
     * @brief Create a deadline from a timeout starting now.
     *
     * Disabled timeouts produce a disabled deadline.
     *
     * @param timeout Timeout duration.
     * @return Deadline instance.
     */
    [[nodiscard]] static Deadline from_timeout(Timeout timeout) noexcept
    {
      if (!timeout.enabled())
      {
        return Deadline{};
      }

      return Deadline{clock::now() + timeout.value()};
    }

    /**
     * @brief Create a deadline from a duration starting now.
     *
     * Non-positive durations produce an already expired deadline.
     *
     * @tparam Rep Duration representation type.
     * @tparam Period Duration period type.
     * @param duration Duration from now.
     * @return Deadline instance.
     */
    template <class Rep, class Period>
    [[nodiscard]] static Deadline after(
        std::chrono::duration<Rep, Period> duration) noexcept
    {
      return Deadline{
          clock::now() +
          std::chrono::duration_cast<clock::duration>(duration)};
    }

    /**
     * @brief Check whether this deadline is enabled.
     *
     * @return true if this deadline stores a real time point.
     */
    [[nodiscard]] constexpr bool enabled() const noexcept
    {
      return enabled_;
    }

    /**
     * @brief Check whether this deadline is disabled.
     *
     * @return true if the deadline never expires.
     */
    [[nodiscard]] constexpr bool disabled_value() const noexcept
    {
      return !enabled_;
    }

    /**
     * @brief Return the stored deadline time point.
     *
     * The value is meaningful only when enabled() returns true.
     *
     * @return Stored time point.
     */
    [[nodiscard]] constexpr time_point time() const noexcept
    {
      return time_;
    }

    /**
     * @brief Check whether the deadline has expired at the current time.
     *
     * Disabled deadlines never expire.
     *
     * @return true if the deadline is enabled and already expired.
     */
    [[nodiscard]] bool expired() const noexcept
    {
      return expired_at(clock::now());
    }

    /**
     * @brief Check whether the deadline has expired at a given time.
     *
     * Disabled deadlines never expire.
     *
     * @param now Time point used for comparison.
     * @return true if now is greater than or equal to the stored deadline.
     */
    [[nodiscard]] constexpr bool expired_at(time_point now) const noexcept
    {
      return enabled_ && now >= time_;
    }

    /**
     * @brief Return the remaining time before the deadline expires.
     *
     * Disabled deadlines return a zero duration because no finite remaining
     * duration exists.
     *
     * Expired deadlines also return zero.
     *
     * @return Remaining duration.
     */
    [[nodiscard]] clock::duration remaining() const noexcept
    {
      if (!enabled_)
      {
        return clock::duration::zero();
      }

      const time_point now = clock::now();

      if (now >= time_)
      {
        return clock::duration::zero();
      }

      return time_ - now;
    }

    /**
     * @brief Return the remaining time in milliseconds.
     *
     * @return Remaining milliseconds, or zero when disabled or expired.
     */
    [[nodiscard]] std::chrono::milliseconds remaining_ms() const noexcept
    {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
          remaining());
    }

  private:
    /**
     * @brief Stored absolute deadline time.
     */
    time_point time_;

    /**
     * @brief Whether this deadline is active.
     */
    bool enabled_;
  };

  /**
   * @brief Compare two deadlines for equality.
   *
   * @param lhs Left deadline.
   * @param rhs Right deadline.
   * @return true if both deadlines have the same enabled state and time point.
   */
  [[nodiscard]] inline constexpr bool operator==(
      Deadline lhs,
      Deadline rhs) noexcept
  {
    return lhs.enabled() == rhs.enabled() && lhs.time() == rhs.time();
  }

  /**
   * @brief Compare two deadlines for inequality.
   *
   * @param lhs Left deadline.
   * @param rhs Right deadline.
   * @return true if the deadlines differ.
   */
  [[nodiscard]] inline constexpr bool operator!=(
      Deadline lhs,
      Deadline rhs) noexcept
  {
    return !(lhs == rhs);
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_DEADLINE_HPP
