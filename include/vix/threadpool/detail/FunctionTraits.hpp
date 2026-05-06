/**
 *
 * @file FunctionTraits.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_FUNCTION_TRAITS_HPP
#define VIX_THREADPOOL_DETAIL_FUNCTION_TRAITS_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>

namespace vix::threadpool::detail
{
  /**
   * @brief Extract callable type information.
   *
   * FunctionTraits exposes the result type, argument tuple, arity, and argument
   * types for free functions, function pointers, member functions, lambdas, and
   * functor objects.
   *
   * @tparam T Callable type.
   */
  template <class T>
  struct FunctionTraits : FunctionTraits<decltype(&T::operator())>
  {
  };

  /**
   * @brief Function traits specialization for plain function types.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class R, class... Args>
  struct FunctionTraits<R(Args...)>
  {
    /**
     * @brief Callable result type.
     */
    using result_type = R;

    /**
     * @brief Tuple containing all callable argument types.
     */
    using argument_tuple = std::tuple<Args...>;

    /**
     * @brief std::function type matching the callable signature.
     */
    using function_type = std::function<R(Args...)>;

    /**
     * @brief Number of callable arguments.
     */
    static constexpr std::size_t arity = sizeof...(Args);

    /**
     * @brief Return the type of one callable argument.
     *
     * @tparam Index Argument index.
     */
    template <std::size_t Index>
    using argument_type = std::tuple_element_t<Index, argument_tuple>;
  };

  /**
   * @brief Function traits specialization for function pointers.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class R, class... Args>
  struct FunctionTraits<R (*)(Args...)> : FunctionTraits<R(Args...)>
  {
  };

  /**
   * @brief Function traits specialization for function references.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class R, class... Args>
  struct FunctionTraits<R (&)(Args...)> : FunctionTraits<R(Args...)>
  {
  };

  /**
   * @brief Function traits specialization for const function pointers.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class R, class... Args>
  struct FunctionTraits<R (*const)(Args...)> : FunctionTraits<R(Args...)>
  {
  };

  /**
   * @brief Function traits specialization for std::function.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class R, class... Args>
  struct FunctionTraits<std::function<R(Args...)>> : FunctionTraits<R(Args...)>
  {
  };

  /**
   * @brief Function traits specialization for non-const member functions.
   *
   * @tparam C Class type.
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class C, class R, class... Args>
  struct FunctionTraits<R (C::*)(Args...)> : FunctionTraits<R(Args...)>
  {
    /**
     * @brief Class that owns the member function.
     */
    using class_type = C;
  };

  /**
   * @brief Function traits specialization for const member functions.
   *
   * @tparam C Class type.
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class C, class R, class... Args>
  struct FunctionTraits<R (C::*)(Args...) const> : FunctionTraits<R(Args...)>
  {
    /**
     * @brief Class that owns the member function.
     */
    using class_type = C;
  };

  /**
   * @brief Function traits specialization for noexcept member functions.
   *
   * @tparam C Class type.
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class C, class R, class... Args>
  struct FunctionTraits<R (C::*)(Args...) noexcept> : FunctionTraits<R(Args...)>
  {
    /**
     * @brief Class that owns the member function.
     */
    using class_type = C;
  };

  /**
   * @brief Function traits specialization for const noexcept member functions.
   *
   * @tparam C Class type.
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class C, class R, class... Args>
  struct FunctionTraits<R (C::*)(Args...) const noexcept> : FunctionTraits<R(Args...)>
  {
    /**
     * @brief Class that owns the member function.
     */
    using class_type = C;
  };

  /**
   * @brief Convenience alias for a callable result type.
   *
   * @tparam T Callable type.
   */
  template <class T>
  using function_result_t = typename FunctionTraits<std::decay_t<T>>::result_type;

  /**
   * @brief Convenience alias for a callable argument tuple.
   *
   * @tparam T Callable type.
   */
  template <class T>
  using function_arguments_t = typename FunctionTraits<std::decay_t<T>>::argument_tuple;

  /**
   * @brief Return the number of arguments accepted by a callable type.
   *
   * @tparam T Callable type.
   */
  template <class T>
  inline constexpr std::size_t function_arity_v =
      FunctionTraits<std::decay_t<T>>::arity;

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_FUNCTION_TRAITS_HPP
