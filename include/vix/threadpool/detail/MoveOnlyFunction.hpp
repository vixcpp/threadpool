/**
 *
 * @file MoveOnlyFunction.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_MOVE_ONLY_FUNCTION_HPP
#define VIX_THREADPOOL_DETAIL_MOVE_ONLY_FUNCTION_HPP

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace vix::threadpool::detail
{
  /**
   * @brief Move-only type-erased callable wrapper.
   *
   * MoveOnlyFunction is similar to std::function, but it supports move-only
   * callables such as lambdas that capture std::unique_ptr, packaged tasks,
   * promises, or other non-copyable state.
   *
   * This is useful inside the threadpool because queued work often owns
   * move-only execution state.
   *
   * @tparam Signature Callable signature.
   */
  template <class Signature>
  class MoveOnlyFunction;

  /**
   * @brief Move-only callable wrapper specialization for function signatures.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   */
  template <class R, class... Args>
  class MoveOnlyFunction<R(Args...)>
  {
  private:
    /**
     * @brief Abstract callable storage interface.
     */
    struct Concept
    {
      /**
       * @brief Virtual destructor.
       */
      virtual ~Concept() = default;

      /**
       * @brief Invoke the stored callable.
       *
       * @param args Arguments forwarded to the callable.
       * @return Callable result.
       */
      virtual R invoke(Args &&...args) = 0;
    };

    /**
     * @brief Concrete callable storage.
     *
     * @tparam Fn Stored callable type.
     */
    template <class Fn>
    struct Model final : Concept
    {
      /**
       * @brief Stored callable.
       */
      Fn fn;

      /**
       * @brief Construct from a callable.
       *
       * @param value Callable to store.
       */
      explicit Model(Fn &&value) noexcept(std::is_nothrow_move_constructible_v<Fn>)
          : fn(std::move(value))
      {
      }

      /**
       * @brief Invoke the stored callable.
       *
       * @param args Arguments forwarded to the callable.
       * @return Callable result.
       */
      R invoke(Args &&...args) override
      {
        if constexpr (std::is_void_v<R>)
        {
          std::invoke(fn, std::forward<Args>(args)...);
          return;
        }
        else
        {
          return std::invoke(fn, std::forward<Args>(args)...);
        }
      }
    };

  public:
    /**
     * @brief Construct an empty callable wrapper.
     */
    MoveOnlyFunction() noexcept = default;

    /**
     * @brief Construct an empty callable wrapper from nullptr.
     */
    MoveOnlyFunction(std::nullptr_t) noexcept
        : callable_(nullptr)
    {
    }

    /**
     * @brief Construct from any compatible callable.
     *
     * @tparam Fn Callable type.
     * @param fn Callable to store.
     */
    template <
        class Fn,
        class Decayed = std::decay_t<Fn>,
        std::enable_if_t<
            !std::is_same_v<Decayed, MoveOnlyFunction> &&
                std::is_invocable_r_v<R, Decayed &, Args...>,
            int> = 0>
    MoveOnlyFunction(Fn &&fn)
        : callable_(std::make_unique<Model<Decayed>>(Decayed(std::forward<Fn>(fn))))
    {
    }

    /**
     * @brief Move-construct a callable wrapper.
     */
    MoveOnlyFunction(MoveOnlyFunction &&) noexcept = default;

    /**
     * @brief Move-assign a callable wrapper.
     *
     * @return Reference to this wrapper.
     */
    MoveOnlyFunction &operator=(MoveOnlyFunction &&) noexcept = default;

    MoveOnlyFunction(const MoveOnlyFunction &) = delete;
    MoveOnlyFunction &operator=(const MoveOnlyFunction &) = delete;

    /**
     * @brief Assign nullptr and clear the stored callable.
     *
     * @return Reference to this wrapper.
     */
    MoveOnlyFunction &operator=(std::nullptr_t) noexcept
    {
      reset();
      return *this;
    }

    /**
     * @brief Assign a new callable.
     *
     * @tparam Fn Callable type.
     * @param fn Callable to store.
     * @return Reference to this wrapper.
     */
    template <
        class Fn,
        class Decayed = std::decay_t<Fn>,
        std::enable_if_t<
            !std::is_same_v<Decayed, MoveOnlyFunction> &&
                std::is_invocable_r_v<R, Decayed &, Args...>,
            int> = 0>
    MoveOnlyFunction &operator=(Fn &&fn)
    {
      MoveOnlyFunction tmp(std::forward<Fn>(fn));
      swap(tmp);
      return *this;
    }

    /**
     * @brief Destroy the callable wrapper.
     */
    ~MoveOnlyFunction() = default;

    /**
     * @brief Invoke the stored callable.
     *
     * @param args Arguments forwarded to the callable.
     * @return Callable result.
     *
     * @throws std::bad_function_call If no callable is stored.
     */
    R operator()(Args... args)
    {
      if (!callable_)
      {
        throw std::bad_function_call();
      }

      if constexpr (std::is_void_v<R>)
      {
        callable_->invoke(std::forward<Args>(args)...);
        return;
      }
      else
      {
        return callable_->invoke(std::forward<Args>(args)...);
      }
    }

    /**
     * @brief Check whether a callable is stored.
     *
     * @return true if this wrapper contains a callable.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return static_cast<bool>(callable_);
    }

    /**
     * @brief Check whether the wrapper is empty.
     *
     * @return true if no callable is stored.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return !callable_;
    }

    /**
     * @brief Clear the stored callable.
     */
    void reset() noexcept
    {
      callable_.reset();
    }

    /**
     * @brief Swap two callable wrappers.
     *
     * @param other Other wrapper.
     */
    void swap(MoveOnlyFunction &other) noexcept
    {
      callable_.swap(other.callable_);
    }

  private:
    /**
     * @brief Type-erased callable storage.
     */
    std::unique_ptr<Concept> callable_;
  };

  /**
   * @brief Swap two MoveOnlyFunction instances.
   *
   * @tparam R Return type.
   * @tparam Args Argument types.
   * @param lhs Left wrapper.
   * @param rhs Right wrapper.
   */
  template <class R, class... Args>
  void swap(
      MoveOnlyFunction<R(Args...)> &lhs,
      MoveOnlyFunction<R(Args...)> &rhs) noexcept
  {
    lhs.swap(rhs);
  }

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_MOVE_ONLY_FUNCTION_HPP
