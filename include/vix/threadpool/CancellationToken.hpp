/**
 *
 * @file CancellationToken.hpp
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
#ifndef VIX_THREADPOOL_CANCELLATION_TOKEN_HPP
#define VIX_THREADPOOL_CANCELLATION_TOKEN_HPP

#include <atomic>
#include <memory>

namespace vix::threadpool
{
  /**
   * @brief Shared cancellation state used by cancellation tokens and sources.
   *
   * CancellationState stores the atomic cancellation flag shared between one
   * CancellationSource and all CancellationToken objects created from it.
   *
   * The state is reference-counted through std::shared_ptr and may be safely
   * observed from multiple threads.
   */
  class CancellationState
  {
  public:
    /**
     * @brief Construct a non-cancelled cancellation state.
     */
    CancellationState() noexcept = default;

    CancellationState(const CancellationState &) = delete;
    CancellationState &operator=(const CancellationState &) = delete;

    /**
     * @brief Request cancellation.
     *
     * This operation is idempotent. Calling it multiple times keeps the state
     * cancelled.
     */
    void request_cancel() noexcept
    {
      cancelled_.store(true, std::memory_order_release);
    }

    /**
     * @brief Check whether cancellation has been requested.
     *
     * @return true if cancellation was requested, false otherwise.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return cancelled_.load(std::memory_order_acquire);
    }

  private:
    /**
     * @brief Atomic cancellation flag.
     */
    std::atomic<bool> cancelled_{false};
  };

  /**
   * @brief Lightweight cancellation observer.
   *
   * CancellationToken is passed to tasks or task options so they can observe
   * cooperative cancellation without owning the cancellation source.
   *
   * A default-constructed token is not connected to any source and therefore
   * never reports cancellation.
   */
  class CancellationToken
  {
  public:
    /**
     * @brief Construct an empty non-cancellable token.
     */
    CancellationToken() noexcept = default;

    /**
     * @brief Construct a token from a shared cancellation state.
     *
     * @param state Shared cancellation state observed by this token.
     */
    explicit CancellationToken(std::shared_ptr<CancellationState> state) noexcept
        : state_(std::move(state))
    {
    }

    /**
     * @brief Check whether this token is connected to a cancellation source.
     *
     * @return true if the token can observe cancellation, false otherwise.
     */
    [[nodiscard]] bool can_cancel() const noexcept
    {
      return static_cast<bool>(state_);
    }

    /**
     * @brief Check whether cancellation has been requested.
     *
     * @return true if the associated source requested cancellation.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return state_ ? state_->cancelled() : false;
    }

    /**
     * @brief Alias for @ref cancelled.
     *
     * @return true if cancellation has been requested.
     */
    [[nodiscard]] bool is_cancelled() const noexcept
    {
      return cancelled();
    }

    /**
     * @brief Check whether execution may continue.
     *
     * @return true if cancellation was not requested, false otherwise.
     */
    [[nodiscard]] bool stop_requested() const noexcept
    {
      return cancelled();
    }

    /**
     * @brief Check whether execution may continue.
     *
     * @return true if cancellation was not requested, false otherwise.
     */
    [[nodiscard]] bool can_continue() const noexcept
    {
      return !cancelled();
    }

    /**
     * @brief Reset the token to an empty non-cancellable state.
     */
    void reset() noexcept
    {
      state_.reset();
    }

  private:
    /**
     * @brief Shared cancellation state observed by this token.
     */
    std::shared_ptr<CancellationState> state_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_CANCELLATION_TOKEN_HPP
