/**
 *
 * @file ThreadName.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_THREAD_NAME_HPP
#define VIX_THREADPOOL_DETAIL_THREAD_NAME_HPP

#include <cstddef>
#include <string>
#include <thread>

namespace vix::threadpool::detail
{
  /**
   * @brief Maximum portable thread name length used by the module.
   *
   * Linux pthread names are limited to 16 bytes including the null terminator.
   * The value below keeps names safe across common platforms.
   */
  inline constexpr std::size_t max_thread_name_length = 15;

  /**
   * @brief Normalize a thread name for platform-specific thread naming APIs.
   *
   * The returned name is never longer than @ref max_thread_name_length.
   *
   * @param name Requested thread name.
   * @return Normalized thread name.
   */
  [[nodiscard]] std::string normalize_thread_name(const std::string &name);

  /**
   * @brief Build a worker thread name from a prefix and numeric index.
   *
   * The returned name is normalized and safe to pass to @ref set_current_thread_name.
   *
   * @param prefix Name prefix.
   * @param index Worker index.
   * @return Normalized worker thread name.
   */
  [[nodiscard]] std::string make_worker_thread_name(
      const std::string &prefix,
      std::size_t index);

  /**
   * @brief Set the name of the current thread when supported by the platform.
   *
   * This function is best-effort:
   * - unsupported platforms are ignored
   * - invalid or too-long names are normalized
   * - platform API failures are not reported
   *
   * @param name Requested current thread name.
   */
  void set_current_thread_name(const std::string &name) noexcept;

  /**
   * @brief Set the name of a given thread when supported by the platform.
   *
   * This function is best-effort:
   * - unsupported platforms are ignored
   * - invalid or too-long names are normalized
   * - platform API failures are not reported
   *
   * @param thread Thread to name.
   * @param name Requested thread name.
   */
  void set_thread_name(std::thread &thread, const std::string &name) noexcept;

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_THREAD_NAME_HPP
