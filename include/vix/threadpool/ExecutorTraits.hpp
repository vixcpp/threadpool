/**
 *
 * @file ExecutorTraits.hpp
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
#ifndef VIX_THREADPOOL_EXECUTOR_TRAITS_HPP
#define VIX_THREADPOOL_EXECUTOR_TRAITS_HPP

#include <type_traits>
#include <utility>

#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/TaskOptions.hpp>

namespace vix::threadpool
{
  /**
   * @brief Detect whether a type exposes post(Fn).
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn, class = void>
  struct HasPost : std::false_type
  {
  };

  /**
   * @brief HasPost specialization for executor-like types with post(Fn).
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  struct HasPost<
      Executor,
      Fn,
      std::void_t<decltype(std::declval<Executor &>().post(std::declval<Fn>()))>> : std::true_type
  {
  };

  /**
   * @brief Detect whether a type exposes submit(Fn).
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn, class = void>
  struct HasSubmit : std::false_type
  {
  };

  /**
   * @brief HasSubmit specialization for executor-like types with submit(Fn).
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  struct HasSubmit<
      Executor,
      Fn,
      std::void_t<decltype(std::declval<Executor &>().submit(std::declval<Fn>()))>> : std::true_type
  {
  };

  /**
   * @brief Detect whether a type exposes submit(Fn, TaskOptions).
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn, class = void>
  struct HasSubmitWithOptions : std::false_type
  {
  };

  /**
   * @brief HasSubmitWithOptions specialization for submit(Fn, TaskOptions).
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  struct HasSubmitWithOptions<
      Executor,
      Fn,
      std::void_t<decltype(std::declval<Executor &>().submit(
          std::declval<Fn>(),
          std::declval<TaskOptions>()))>> : std::true_type
  {
  };

  /**
   * @brief Detect whether a type exposes shutdown().
   *
   * @tparam Executor Executor-like type.
   */
  template <class Executor, class = void>
  struct HasShutdown : std::false_type
  {
  };

  /**
   * @brief HasShutdown specialization for executor-like types with shutdown().
   *
   * @tparam Executor Executor-like type.
   */
  template <class Executor>
  struct HasShutdown<
      Executor,
      std::void_t<decltype(std::declval<Executor &>().shutdown())>> : std::true_type
  {
  };

  /**
   * @brief Detect whether a type exposes wait_idle().
   *
   * @tparam Executor Executor-like type.
   */
  template <class Executor, class = void>
  struct HasWaitIdle : std::false_type
  {
  };

  /**
   * @brief HasWaitIdle specialization for executor-like types with wait_idle().
   *
   * @tparam Executor Executor-like type.
   */
  template <class Executor>
  struct HasWaitIdle<
      Executor,
      std::void_t<decltype(std::declval<Executor &>().wait_idle())>> : std::true_type
  {
  };

  /**
   * @brief Convenience value for HasPost.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  inline constexpr bool has_post_v = HasPost<Executor, Fn>::value;

  /**
   * @brief Convenience value for HasSubmit.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  inline constexpr bool has_submit_v = HasSubmit<Executor, Fn>::value;

  /**
   * @brief Convenience value for HasSubmitWithOptions.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  inline constexpr bool has_submit_with_options_v =
      HasSubmitWithOptions<Executor, Fn>::value;

  /**
   * @brief Convenience value for HasShutdown.
   *
   * @tparam Executor Executor-like type.
   */
  template <class Executor>
  inline constexpr bool has_shutdown_v = HasShutdown<Executor>::value;

  /**
   * @brief Convenience value for HasWaitIdle.
   *
   * @tparam Executor Executor-like type.
   */
  template <class Executor>
  inline constexpr bool has_wait_idle_v = HasWaitIdle<Executor>::value;

  /**
   * @brief Detect whether a type satisfies the basic executor shape.
   *
   * A basic executor can at least post fire-and-forget work.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  struct IsBasicExecutor
      : std::bool_constant<has_post_v<Executor, Fn>>
  {
  };

  /**
   * @brief Detect whether a type satisfies the future executor shape.
   *
   * A future executor can submit work and return a future-like result.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  struct IsFutureExecutor
      : std::bool_constant<
            has_submit_v<Executor, Fn> ||
            has_submit_with_options_v<Executor, Fn>>
  {
  };

  /**
   * @brief Convenience value for IsBasicExecutor.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  inline constexpr bool is_basic_executor_v =
      IsBasicExecutor<Executor, Fn>::value;

  /**
   * @brief Convenience value for IsFutureExecutor.
   *
   * @tparam Executor Executor-like type.
   * @tparam Fn Callable type.
   */
  template <class Executor, class Fn>
  inline constexpr bool is_future_executor_v =
      IsFutureExecutor<Executor, Fn>::value;

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_EXECUTOR_TRAITS_HPP
