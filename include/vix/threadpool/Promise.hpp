/**
 *
 * @file Promise.hpp
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
#ifndef VIX_THREADPOOL_PROMISE_HPP
#define VIX_THREADPOOL_PROMISE_HPP

#include <exception>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>

#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/SharedState.hpp>
#include <vix/threadpool/ThreadPoolError.hpp>

namespace vix::threadpool
{
  /**
   * @brief Producer side of a threadpool asynchronous result.
   *
   * Promise owns a shared state and is used by task execution code to publish a
   * value, exception, or error. The matching Future is used by callers to wait
   * for and retrieve that result.
   *
   * @tparam T Result type.
   */
  template <class T>
  class Promise
  {
  public:
    /**
     * @brief Result value type.
     */
    using value_type = T;

    /**
     * @brief Construct a promise with a new shared state.
     */
    Promise()
        : state_(std::make_shared<SharedState<T>>()),
          future_obtained_(false)
    {
    }

    /**
     * @brief Construct a promise from an existing shared state.
     *
     * @param state Shared state owned by this promise.
     */
    explicit Promise(std::shared_ptr<SharedState<T>> state)
        : state_(std::move(state)),
          future_obtained_(false)
    {
      if (!state_)
      {
        state_ = std::make_shared<SharedState<T>>();
      }
    }

    Promise(const Promise &) = delete;
    Promise &operator=(const Promise &) = delete;

    /**
     * @brief Move-construct a promise.
     */
    Promise(Promise &&) noexcept = default;

    /**
     * @brief Move-assign a promise.
     *
     * @return Reference to this promise.
     */
    Promise &operator=(Promise &&) noexcept = default;

    /**
     * @brief Destroy the promise.
     */
    ~Promise() = default;

    /**
     * @brief Check whether this promise owns a shared state.
     *
     * @return true if the promise has a state.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return static_cast<bool>(state_);
    }

    /**
     * @brief Create the future associated with this promise.
     *
     * This function may be called only once.
     *
     * @return Future observing the same shared state.
     *
     * @throws std::future_error If the future was already retrieved or the
     * promise has no state.
     */
    [[nodiscard]] Future<T> get_future()
    {
      ensure_valid();

      if (future_obtained_)
      {
        throw std::future_error(std::future_errc::future_already_retrieved);
      }

      future_obtained_ = true;
      return Future<T>{state_};
    }

    /**
     * @brief Store a value in the shared state.
     *
     * @param value Value to store.
     */
    void set_value(T value)
    {
      ensure_valid();
      state_->set_value(std::move(value));
    }

    /**
     * @brief Construct the value in-place in the shared state.
     *
     * @tparam Args Constructor argument types.
     * @param args Constructor arguments forwarded to T.
     */
    template <class... Args>
    void emplace_value(Args &&...args)
    {
      ensure_valid();
      state_->emplace_value(std::forward<Args>(args)...);
    }

    /**
     * @brief Store an exception in the shared state.
     *
     * @param exception Exception to store.
     */
    void set_exception(std::exception_ptr exception)
    {
      ensure_valid();
      state_->set_exception(exception);
    }

    /**
     * @brief Store the current exception in the shared state.
     */
    void set_current_exception()
    {
      set_exception(std::current_exception());
    }

    /**
     * @brief Store a threadpool error in the shared state.
     *
     * @param error Threadpool error code.
     */
    void set_error(ThreadPoolErrc error)
    {
      ensure_valid();
      state_->set_error(error);
    }

    /**
     * @brief Return the shared state.
     *
     * @return Shared state pointer.
     */
    [[nodiscard]] std::shared_ptr<SharedState<T>> state() const noexcept
    {
      return state_;
    }

  private:
    /**
     * @brief Throw if this promise has no shared state.
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
     * @brief Whether get_future() has already been called.
     */
    bool future_obtained_;
  };

  /**
   * @brief Promise specialization for void results.
   */
  template <>
  class Promise<void>
  {
  public:
    /**
     * @brief Result value type.
     */
    using value_type = void;

    /**
     * @brief Construct a promise with a new shared state.
     */
    Promise()
        : state_(std::make_shared<SharedState<void>>()),
          future_obtained_(false)
    {
    }

    /**
     * @brief Construct a promise from an existing shared state.
     *
     * @param state Shared state owned by this promise.
     */
    explicit Promise(std::shared_ptr<SharedState<void>> state)
        : state_(std::move(state)),
          future_obtained_(false)
    {
      if (!state_)
      {
        state_ = std::make_shared<SharedState<void>>();
      }
    }

    Promise(const Promise &) = delete;
    Promise &operator=(const Promise &) = delete;

    /**
     * @brief Move-construct a promise.
     */
    Promise(Promise &&) noexcept = default;

    /**
     * @brief Move-assign a promise.
     *
     * @return Reference to this promise.
     */
    Promise &operator=(Promise &&) noexcept = default;

    /**
     * @brief Destroy the promise.
     */
    ~Promise() = default;

    /**
     * @brief Check whether this promise owns a shared state.
     *
     * @return true if the promise has a state.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return static_cast<bool>(state_);
    }

    /**
     * @brief Create the future associated with this promise.
     *
     * This function may be called only once.
     *
     * @return Future observing the same shared state.
     *
     * @throws std::future_error If the future was already retrieved or the
     * promise has no state.
     */
    [[nodiscard]] Future<void> get_future()
    {
      ensure_valid();

      if (future_obtained_)
      {
        throw std::future_error(std::future_errc::future_already_retrieved);
      }

      future_obtained_ = true;
      return Future<void>{state_};
    }

    /**
     * @brief Mark the shared state as successfully completed.
     */
    void set_value()
    {
      ensure_valid();
      state_->set_value();
    }

    /**
     * @brief Store an exception in the shared state.
     *
     * @param exception Exception to store.
     */
    void set_exception(std::exception_ptr exception)
    {
      ensure_valid();
      state_->set_exception(exception);
    }

    /**
     * @brief Store the current exception in the shared state.
     */
    void set_current_exception()
    {
      set_exception(std::current_exception());
    }

    /**
     * @brief Store a threadpool error in the shared state.
     *
     * @param error Threadpool error code.
     */
    void set_error(ThreadPoolErrc error)
    {
      ensure_valid();
      state_->set_error(error);
    }

    /**
     * @brief Return the shared state.
     *
     * @return Shared state pointer.
     */
    [[nodiscard]] std::shared_ptr<SharedState<void>> state() const noexcept
    {
      return state_;
    }

  private:
    /**
     * @brief Throw if this promise has no shared state.
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
    std::shared_ptr<SharedState<void>> state_;

    /**
     * @brief Whether get_future() has already been called.
     */
    bool future_obtained_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PROMISE_HPP
