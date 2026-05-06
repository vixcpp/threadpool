/**
 *
 * @file Future.hpp
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
#ifndef VIX_THREADPOOL_FUTURE_HPP
#define VIX_THREADPOOL_FUTURE_HPP

#include <chrono>
#include <future>
#include <memory>
#include <utility>

#include <vix/threadpool/SharedState.hpp>
#include <vix/threadpool/TaskResult.hpp>
#include <vix/threadpool/TaskStatus.hpp>
#include <vix/threadpool/ThreadPoolError.hpp>

namespace vix::threadpool
{
  template <class T>
  class Promise;

  /**
   * @brief Future representing the eventual result of a threadpool task.
   *
   * Future is move-only and shares state with a Promise. It provides a minimal
   * std::future-like API while also exposing threadpool-specific status, result,
   * and error information.
   *
   * @tparam T Result type.
   */
  template <class T>
  class Future
  {
  public:
    /**
     * @brief Result value type.
     */
    using value_type = T;

    /**
     * @brief Construct an invalid future.
     */
    Future() noexcept = default;

    /**
     * @brief Construct a future from shared state.
     *
     * @param state Shared state observed by this future.
     */
    explicit Future(std::shared_ptr<SharedState<T>> state) noexcept
        : state_(std::move(state))
    {
    }

    Future(const Future &) = delete;
    Future &operator=(const Future &) = delete;

    /**
     * @brief Move-construct a future.
     */
    Future(Future &&) noexcept = default;

    /**
     * @brief Move-assign a future.
     *
     * @return Reference to this future.
     */
    Future &operator=(Future &&) noexcept = default;

    /**
     * @brief Destroy the future.
     */
    ~Future() = default;

    /**
     * @brief Check whether this future owns a shared state.
     *
     * @return true if the future is valid.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return static_cast<bool>(state_);
    }

    /**
     * @brief Bool conversion.
     *
     * @return true if the future is valid.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return valid();
    }

    /**
     * @brief Wait until the future becomes ready.
     *
     * @throws std::future_error If the future has no state.
     */
    void wait() const
    {
      ensure_valid();
      state_->wait();
    }

    /**
     * @brief Check whether the future result is ready.
     *
     * @return true if the future is valid and ready.
     */
    [[nodiscard]] bool ready() const
    {
      return state_ ? state_->ready() : false;
    }

    /**
     * @brief Wait until the future becomes ready or the duration expires.
     *
     * @tparam Rep Duration representation type.
     * @tparam Period Duration period type.
     * @param duration Maximum duration to wait.
     * @return std::future_status::ready if ready, timeout otherwise.
     */
    template <class Rep, class Period>
    [[nodiscard]] std::future_status wait_for(
        const std::chrono::duration<Rep, Period> &duration) const
    {
      ensure_valid();

      const auto deadline = std::chrono::steady_clock::now() + duration;
      while (!state_->ready())
      {
        if (std::chrono::steady_clock::now() >= deadline)
        {
          return std::future_status::timeout;
        }

        std::this_thread::yield();
      }

      return std::future_status::ready;
    }

    /**
     * @brief Wait until the future becomes ready or the time point is reached.
     *
     * @tparam Clock Clock type.
     * @tparam Duration Time point duration type.
     * @param time Maximum time point to wait until.
     * @return std::future_status::ready if ready, timeout otherwise.
     */
    template <class Clock, class Duration>
    [[nodiscard]] std::future_status wait_until(
        const std::chrono::time_point<Clock, Duration> &time) const
    {
      ensure_valid();

      while (!state_->ready())
      {
        if (Clock::now() >= time)
        {
          return std::future_status::timeout;
        }

        std::this_thread::yield();
      }

      return std::future_status::ready;
    }

    /**
     * @brief Wait and retrieve the result.
     *
     * This consumes the result. Calling get() more than once throws
     * std::future_error.
     *
     * @return Stored value.
     */
    T get()
    {
      ensure_valid();
      return state_->get();
    }

    /**
     * @brief Return the stored task status.
     *
     * Invalid futures report TaskStatus::created.
     *
     * @return Task status.
     */
    [[nodiscard]] TaskStatus status() const
    {
      return state_ ? state_->status() : TaskStatus::created;
    }

    /**
     * @brief Return the stored task result.
     *
     * Invalid futures report TaskResult::none.
     *
     * @return Task result.
     */
    [[nodiscard]] TaskResult result() const
    {
      return state_ ? state_->result() : TaskResult::none;
    }

    /**
     * @brief Return the stored error code.
     *
     * Invalid futures report ThreadPoolErrc::not_ready.
     *
     * @return Threadpool error code.
     */
    [[nodiscard]] ThreadPoolErrc error() const
    {
      return state_ ? state_->error() : ThreadPoolErrc::not_ready;
    }

  private:
    /**
     * @brief Throw if this future has no shared state.
     */
    void ensure_valid() const
    {
      if (!state_)
      {
        throw std::future_error(std::future_errc::no_state);
      }
    }

  private:
    /**
     * @brief Shared asynchronous state.
     */
    std::shared_ptr<SharedState<T>> state_;

    /**
     * @brief Promise is allowed to create futures from states.
     */
    friend class Promise<T>;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_FUTURE_HPP
