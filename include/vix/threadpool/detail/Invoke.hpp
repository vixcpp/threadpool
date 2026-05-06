/**
 *
 * @file Invoke.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_INVOKE_HPP
#define VIX_THREADPOOL_DETAIL_INVOKE_HPP

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace vix::threadpool::detail
{
  /**
   * @brief Invoke a callable with forwarded arguments.
   *
   * This helper centralizes callable invocation for the threadpool internals.
   * It uses std::invoke so it supports functions, lambdas, functors, member
   * functions, and member data pointers.
   *
   * @tparam Fn Callable type.
   * @tparam Args Callable argument types.
   * @param fn Callable to invoke.
   * @param args Arguments forwarded to the callable.
   * @return Result produced by the callable.
   */
  template <class Fn, class... Args>
  decltype(auto) invoke(Fn &&fn, Args &&...args) noexcept(
      noexcept(std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)))
  {
    return std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
  }

  /**
   * @brief Check whether a callable can be invoked with given arguments.
   *
   * @tparam Fn Callable type.
   * @tparam Args Callable argument types.
   */
  template <class Fn, class... Args>
  inline constexpr bool is_invocable_v = std::is_invocable_v<Fn, Args...>;

  /**
   * @brief Check whether invoking a callable produces a specific result type.
   *
   * @tparam Result Expected result type.
   * @tparam Fn Callable type.
   * @tparam Args Callable argument types.
   */
  template <class Result, class Fn, class... Args>
  inline constexpr bool is_invocable_r_v =
      std::is_invocable_r_v<Result, Fn, Args...>;

  /**
   * @brief Check whether invoking a callable cannot throw.
   *
   * @tparam Fn Callable type.
   * @tparam Args Callable argument types.
   */
  template <class Fn, class... Args>
  inline constexpr bool is_nothrow_invocable_v =
      std::is_nothrow_invocable_v<Fn, Args...>;

  /**
   * @brief Result type produced by invoking a callable.
   *
   * @tparam Fn Callable type.
   * @tparam Args Callable argument types.
   */
  template <class Fn, class... Args>
  using invoke_result_t = std::invoke_result_t<Fn, Args...>;

  /**
   * @brief Invoke a callable and capture any thrown exception.
   *
   * This helper is useful for worker code that must never let exceptions escape
   * from a thread entry point.
   *
   * @tparam Fn Callable type.
   * @param fn Callable to invoke.
   * @return Captured exception pointer, or nullptr on success.
   */
  template <class Fn>
  [[nodiscard]] std::exception_ptr invoke_catching(Fn &&fn) noexcept
  {
    try
    {
      std::invoke(std::forward<Fn>(fn));
      return nullptr;
    }
    catch (...)
    {
      return std::current_exception();
    }
  }

  /**
   * @brief Invoke a callable and ignore any thrown exception.
   *
   * This helper is intended for shutdown paths, cleanup callbacks, and
   * fire-and-forget execution paths where exceptions must not escape.
   *
   * @tparam Fn Callable type.
   * @param fn Callable to invoke.
   */
  template <class Fn>
  void invoke_ignoring_exceptions(Fn &&fn) noexcept
  {
    try
    {
      std::invoke(std::forward<Fn>(fn));
    }
    catch (...)
    {
    }
  }

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_INVOKE_HPP
