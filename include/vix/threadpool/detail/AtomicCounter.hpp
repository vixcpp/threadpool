/**
 *
 * @file AtomicCounter.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_ATOMIC_COUNTER_HPP
#define VIX_THREADPOOL_DETAIL_ATOMIC_COUNTER_HPP

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace vix::threadpool::detail
{
  /**
   * @brief Small typed wrapper around std::atomic for monotonically updated counters.
   *
   * AtomicCounter is used by the threadpool internals for metrics, task ids,
   * worker counters, and lightweight state tracking.
   *
   * The class intentionally exposes only simple counter operations instead of
   * the full std::atomic API.
   *
   * @tparam T Integral counter type.
   */
  template <class T = std::uint64_t>
  class AtomicCounter
  {
    static_assert(std::is_integral_v<T>,
                  "AtomicCounter<T> requires an integral type");

  public:
    /**
     * @brief Counter value type.
     */
    using value_type = T;

    /**
     * @brief Construct a counter initialized to zero.
     */
    constexpr AtomicCounter() noexcept
        : value_(0)
    {
    }

    /**
     * @brief Construct a counter with an initial value.
     *
     * @param initial Initial counter value.
     */
    explicit constexpr AtomicCounter(T initial) noexcept
        : value_(initial)
    {
    }

    AtomicCounter(const AtomicCounter &) = delete;
    AtomicCounter &operator=(const AtomicCounter &) = delete;

    /**
     * @brief Load the current counter value.
     *
     * @param order Memory ordering used for the load operation.
     * @return Current counter value.
     */
    [[nodiscard]] T load(
        std::memory_order order = std::memory_order_relaxed) const noexcept
    {
      return value_.load(order);
    }

    /**
     * @brief Store a new counter value.
     *
     * @param value New value.
     * @param order Memory ordering used for the store operation.
     */
    void store(
        T value,
        std::memory_order order = std::memory_order_relaxed) noexcept
    {
      value_.store(value, order);
    }

    /**
     * @brief Increment the counter by one.
     *
     * @param order Memory ordering used for the operation.
     * @return Value before increment.
     */
    T increment(
        std::memory_order order = std::memory_order_relaxed) noexcept
    {
      return value_.fetch_add(1, order);
    }

    /**
     * @brief Decrement the counter by one.
     *
     * @param order Memory ordering used for the operation.
     * @return Value before decrement.
     */
    T decrement(
        std::memory_order order = std::memory_order_relaxed) noexcept
    {
      return value_.fetch_sub(1, order);
    }

    /**
     * @brief Add a value to the counter.
     *
     * @param value Value to add.
     * @param order Memory ordering used for the operation.
     * @return Value before addition.
     */
    T add(
        T value,
        std::memory_order order = std::memory_order_relaxed) noexcept
    {
      return value_.fetch_add(value, order);
    }

    /**
     * @brief Subtract a value from the counter.
     *
     * @param value Value to subtract.
     * @param order Memory ordering used for the operation.
     * @return Value before subtraction.
     */
    T subtract(
        T value,
        std::memory_order order = std::memory_order_relaxed) noexcept
    {
      return value_.fetch_sub(value, order);
    }

    /**
     * @brief Reset the counter to zero.
     *
     * @param order Memory ordering used for the store operation.
     */
    void reset(
        std::memory_order order = std::memory_order_relaxed) noexcept
    {
      value_.store(0, order);
    }

    /**
     * @brief Check whether the counter is currently zero.
     *
     * @return true if the current value is zero, false otherwise.
     */
    [[nodiscard]] bool zero() const noexcept
    {
      return load() == 0;
    }

  private:
    /**
     * @brief Underlying atomic value.
     */
    std::atomic<T> value_;
  };

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_ATOMIC_COUNTER_HPP
