/**
 *
 * @file Timeout.hpp
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
#ifndef VIX_THREADPOOL_TIMEOUT_HPP
#define VIX_THREADPOOL_TIMEOUT_HPP

#include <chrono>

namespace vix::threadpool
{
  /**
   * @brief Timeout duration used to observe task execution time.
   *
   * Timeout is a lightweight wrapper around std::chrono::milliseconds.
   * It is used by task options and execution logic to describe how long a task
   * is expected to run before it is considered timed out.
   *
   * A zero timeout means timeout observation is disabled.
   */
  class Timeout
  {
  public:
    /**
     * @brief Duration type used by Timeout.
     */
    using duration = std::chrono::milliseconds;

    /**
     * @brief Construct a disabled timeout.
     */
    constexpr Timeout() noexcept
        : value_(duration{0})
    {
    }

    /**
     * @brief Construct a timeout from a duration.
     *
     * Negative durations are normalized to zero.
     *
     * @param value Timeout duration.
     */
    explicit constexpr Timeout(duration value) noexcept
        : value_(normalize(value))
    {
    }

    /**
     * @brief Create a disabled timeout.
     *
     * @return Disabled timeout.
     */
    [[nodiscard]] static constexpr Timeout disabled() noexcept
    {
      return Timeout{};
    }

    /**
     * @brief Create a timeout from milliseconds.
     *
     * @param value Timeout value in milliseconds.
     * @return Timeout instance.
     */
    [[nodiscard]] static constexpr Timeout milliseconds(long long value) noexcept
    {
      return Timeout{duration{value}};
    }

    /**
     * @brief Create a timeout from seconds.
     *
     * @param value Timeout value in seconds.
     * @return Timeout instance.
     */
    [[nodiscard]] static constexpr Timeout seconds(long long value) noexcept
    {
      return Timeout{
          std::chrono::duration_cast<duration>(std::chrono::seconds{value})};
    }

    /**
     * @brief Check whether timeout observation is enabled.
     *
     * @return true if the timeout duration is greater than zero.
     */
    [[nodiscard]] constexpr bool enabled() const noexcept
    {
      return value_.count() > 0;
    }

    /**
     * @brief Check whether timeout observation is disabled.
     *
     * @return true if the timeout duration is zero.
     */
    [[nodiscard]] constexpr bool disabled_value() const noexcept
    {
      return !enabled();
    }

    /**
     * @brief Return the timeout duration.
     *
     * @return Timeout duration.
     */
    [[nodiscard]] constexpr duration value() const noexcept
    {
      return value_;
    }

    /**
     * @brief Return the timeout in milliseconds.
     *
     * @return Timeout value in milliseconds.
     */
    [[nodiscard]] constexpr long long count() const noexcept
    {
      return value_.count();
    }

    /**
     * @brief Check whether an elapsed duration exceeded this timeout.
     *
     * Disabled timeouts never expire.
     *
     * @tparam Rep Duration representation type.
     * @tparam Period Duration period type.
     * @param elapsed Elapsed duration to inspect.
     * @return true if elapsed is greater than this timeout.
     */
    template <class Rep, class Period>
    [[nodiscard]] constexpr bool expired(
        std::chrono::duration<Rep, Period> elapsed) const noexcept
    {
      if (!enabled())
      {
        return false;
      }

      return std::chrono::duration_cast<duration>(elapsed) > value_;
    }

    /**
     * @brief Normalize a timeout duration.
     *
     * Negative values are converted to zero.
     *
     * @param value Input duration.
     * @return Normalized duration.
     */
    [[nodiscard]] static constexpr duration normalize(duration value) noexcept
    {
      return value.count() < 0 ? duration{0} : value;
    }

  private:
    /**
     * @brief Stored timeout duration.
     */
    duration value_;
  };

  /**
   * @brief Compare two timeout values for equality.
   *
   * @param lhs Left timeout.
   * @param rhs Right timeout.
   * @return true if both timeouts store the same duration.
   */
  [[nodiscard]] inline constexpr bool operator==(
      Timeout lhs,
      Timeout rhs) noexcept
  {
    return lhs.value() == rhs.value();
  }

  /**
   * @brief Compare two timeout values for inequality.
   *
   * @param lhs Left timeout.
   * @param rhs Right timeout.
   * @return true if the timeouts store different durations.
   */
  [[nodiscard]] inline constexpr bool operator!=(
      Timeout lhs,
      Timeout rhs) noexcept
  {
    return !(lhs == rhs);
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TIMEOUT_HPP
