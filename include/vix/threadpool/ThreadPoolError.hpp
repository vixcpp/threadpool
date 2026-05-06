/**
 *
 * @file ThreadPoolError.hpp
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
#ifndef VIX_THREADPOOL_THREAD_POOL_ERROR_HPP
#define VIX_THREADPOOL_THREAD_POOL_ERROR_HPP

#include <cstdint>
#include <string>
#include <system_error>
#include <type_traits>

namespace vix::threadpool
{
  /**
   * @brief Error codes reported by the threadpool module.
   *
   * ThreadPoolErrc values are stable and can be converted to std::error_code.
   */
  enum class ThreadPoolErrc : std::uint8_t
  {
    /**
     * @brief No error.
     */
    ok = 0,

    /**
     * @brief An API received an invalid argument.
     */
    invalid_argument = 1,

    /**
     * @brief The thread pool has been stopped or shut down.
     */
    stopped = 2,

    /**
     * @brief The task was rejected before being queued.
     */
    rejected = 3,

    /**
     * @brief The task queue reached its configured capacity.
     */
    queue_full = 4,

    /**
     * @brief The operation timed out.
     */
    timeout = 5,

    /**
     * @brief The operation was cancelled.
     */
    cancelled = 6,

    /**
     * @brief The operation cannot complete yet.
     */
    not_ready = 7,

    /**
     * @brief The operation is not supported.
     */
    not_supported = 8,

    /**
     * @brief An internal threadpool error occurred.
     */
    internal_error = 9
  };

  /**
   * @brief Error category used by the threadpool module.
   */
  class ThreadPoolErrorCategory final : public std::error_category
  {
  public:
    /**
     * @brief Return the category name.
     *
     * @return Static category name.
     */
    [[nodiscard]] const char *name() const noexcept override
    {
      return "vix.threadpool";
    }

    /**
     * @brief Return a readable error message.
     *
     * @param value Numeric error code value.
     * @return Error message.
     */
    [[nodiscard]] std::string message(int value) const override
    {
      switch (static_cast<ThreadPoolErrc>(value))
      {
      case ThreadPoolErrc::ok:
        return "ok";
      case ThreadPoolErrc::invalid_argument:
        return "invalid argument";
      case ThreadPoolErrc::stopped:
        return "thread pool stopped";
      case ThreadPoolErrc::rejected:
        return "task rejected";
      case ThreadPoolErrc::queue_full:
        return "task queue full";
      case ThreadPoolErrc::timeout:
        return "operation timed out";
      case ThreadPoolErrc::cancelled:
        return "operation cancelled";
      case ThreadPoolErrc::not_ready:
        return "operation not ready";
      case ThreadPoolErrc::not_supported:
        return "operation not supported";
      case ThreadPoolErrc::internal_error:
        return "internal thread pool error";
      }

      return "unknown thread pool error";
    }
  };

  /**
   * @brief Return the singleton threadpool error category.
   *
   * @return Threadpool error category.
   */
  [[nodiscard]] inline const std::error_category &threadpool_category() noexcept
  {
    static ThreadPoolErrorCategory category;
    return category;
  }

  /**
   * @brief Create a std::error_code from a threadpool error code.
   *
   * @param error Threadpool error code.
   * @return Matching std::error_code.
   */
  [[nodiscard]] inline std::error_code make_error_code(ThreadPoolErrc error) noexcept
  {
    return {static_cast<int>(error), threadpool_category()};
  }

  /**
   * @brief Check whether a threadpool error code represents success.
   *
   * @param error Threadpool error code.
   * @return true if the error code is ok, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_ok(ThreadPoolErrc error) noexcept
  {
    return error == ThreadPoolErrc::ok;
  }

  /**
   * @brief Check whether a threadpool error code represents failure.
   *
   * @param error Threadpool error code.
   * @return true if the error code is not ok, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_error(ThreadPoolErrc error) noexcept
  {
    return error != ThreadPoolErrc::ok;
  }

} // namespace vix::threadpool

namespace std
{
  /**
   * @brief Enable implicit conversion of ThreadPoolErrc to std::error_code.
   */
  template <>
  struct is_error_code_enum<vix::threadpool::ThreadPoolErrc> : true_type
  {
  };
} // namespace std

#endif // VIX_THREADPOOL_THREAD_POOL_ERROR_HPP
