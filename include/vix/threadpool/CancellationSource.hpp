/**
 *
 * @file CancellationSource.hpp
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
#ifndef VIX_THREADPOOL_CANCELLATION_SOURCE_HPP
#define VIX_THREADPOOL_CANCELLATION_SOURCE_HPP

#include <memory>
#include <utility>

#include <vix/threadpool/CancellationToken.hpp>

namespace vix::threadpool
{
  /**
   * @brief Owner object used to request cooperative cancellation.
   *
   * CancellationSource owns a shared cancellation state and creates
   * CancellationToken objects that observe that state.
   *
   * The source can be copied. Copies share the same cancellation state, so a
   * cancellation request made through one copy is visible through all tokens and
   * source copies that reference the same state.
   */
  class CancellationSource
  {
  public:
    /**
     * @brief Construct a new cancellation source.
     *
     * The source starts in a non-cancelled state.
     */
    CancellationSource()
        : state_(std::make_shared<CancellationState>())
    {
    }

    /**
     * @brief Construct a source from an existing cancellation state.
     *
     * @param state Shared cancellation state.
     */
    explicit CancellationSource(std::shared_ptr<CancellationState> state) noexcept
        : state_(std::move(state))
    {
      if (!state_)
      {
        state_ = std::make_shared<CancellationState>();
      }
    }

    /**
     * @brief Create a token linked to this source.
     *
     * @return Cancellation token observing this source.
     */
    [[nodiscard]] CancellationToken token() const noexcept
    {
      return CancellationToken{state_};
    }

    /**
     * @brief Request cancellation for all linked tokens.
     *
     * This operation is idempotent.
     */
    void request_cancel() noexcept
    {
      if (state_)
      {
        state_->request_cancel();
      }
    }

    /**
     * @brief Check whether cancellation has been requested.
     *
     * @return true if this source is already cancelled.
     */
    [[nodiscard]] bool cancelled() const noexcept
    {
      return state_ ? state_->cancelled() : false;
    }

    /**
     * @brief Alias for @ref cancelled.
     *
     * @return true if this source is already cancelled.
     */
    [[nodiscard]] bool is_cancelled() const noexcept
    {
      return cancelled();
    }

    /**
     * @brief Check whether the source owns a cancellation state.
     *
     * @return true if this source is valid.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return static_cast<bool>(state_);
    }

    /**
     * @brief Reset this source to a new non-cancelled state.
     *
     * Existing tokens keep observing the previous state.
     */
    void reset()
    {
      state_ = std::make_shared<CancellationState>();
    }

  private:
    /**
     * @brief Shared cancellation state owned by this source.
     */
    std::shared_ptr<CancellationState> state_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_CANCELLATION_SOURCE_HPP
