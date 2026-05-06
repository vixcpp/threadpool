/**
 *
 * @file ScopeExit.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_SCOPE_EXIT_HPP
#define VIX_THREADPOOL_DETAIL_SCOPE_EXIT_HPP

#include <type_traits>
#include <utility>

namespace vix::threadpool::detail
{
  /**
   * @brief RAII helper that executes a callable when leaving scope.
   *
   * ScopeExit is used to guarantee cleanup in internal code paths such as
   * worker shutdown, counter rollback, state restoration, and lock-safe exits.
   *
   * The stored callable is executed in the destructor if the guard is still
   * active. The destructor never allows exceptions to escape.
   *
   * @tparam Fn Callable type executed on scope exit.
   */
  template <class Fn>
  class ScopeExit
  {
    static_assert(std::is_move_constructible_v<Fn>,
                  "ScopeExit requires a move-constructible callable");

  public:
    /**
     * @brief Construct a scope-exit guard.
     *
     * @param fn Callable executed when the guard is destroyed.
     */
    explicit ScopeExit(Fn fn) noexcept(std::is_nothrow_move_constructible_v<Fn>)
        : fn_(std::move(fn)),
          active_(true)
    {
    }

    /**
     * @brief Move-construct a scope-exit guard.
     *
     * The moved-from guard is released so only one guard owns the cleanup action.
     *
     * @param other Source guard.
     */
    ScopeExit(ScopeExit &&other) noexcept(std::is_nothrow_move_constructible_v<Fn>)
        : fn_(std::move(other.fn_)),
          active_(other.active_)
    {
      other.release();
    }

    ScopeExit(const ScopeExit &) = delete;
    ScopeExit &operator=(const ScopeExit &) = delete;
    ScopeExit &operator=(ScopeExit &&) = delete;

    /**
     * @brief Execute the cleanup callable if the guard is active.
     */
    ~ScopeExit() noexcept
    {
      if (!active_)
      {
        return;
      }

      try
      {
        fn_();
      }
      catch (...)
      {
      }
    }

    /**
     * @brief Disable the cleanup action.
     *
     * After release(), destroying this guard will not execute the callable.
     */
    void release() noexcept
    {
      active_ = false;
    }

    /**
     * @brief Check whether the guard is still active.
     *
     * @return true if the cleanup callable will run on destruction.
     */
    [[nodiscard]] bool active() const noexcept
    {
      return active_;
    }

  private:
    /**
     * @brief Cleanup callable.
     */
    Fn fn_;

    /**
     * @brief Whether the cleanup callable should run on destruction.
     */
    bool active_;
  };

  /**
   * @brief Create a ScopeExit guard with type deduction.
   *
   * @tparam Fn Callable type.
   * @param fn Callable executed when the returned guard is destroyed.
   * @return ScopeExit guard.
   */
  template <class Fn>
  [[nodiscard]] ScopeExit<std::decay_t<Fn>> make_scope_exit(Fn &&fn)
  {
    return ScopeExit<std::decay_t<Fn>>(std::forward<Fn>(fn));
  }

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_SCOPE_EXIT_HPP
