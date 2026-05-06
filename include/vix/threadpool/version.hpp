/**
 *
 * @file version.hpp
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
#ifndef VIX_THREADPOOL_VERSION_HPP
#define VIX_THREADPOOL_VERSION_HPP

/**
 * @brief Major version of the Vix threadpool module.
 */
#define VIX_THREADPOOL_VERSION_MAJOR 0

/**
 * @brief Minor version of the Vix threadpool module.
 */
#define VIX_THREADPOOL_VERSION_MINOR 1

/**
 * @brief Patch version of the Vix threadpool module.
 */
#define VIX_THREADPOOL_VERSION_PATCH 0

/**
 * @brief Complete semantic version string of the Vix threadpool module.
 */
#define VIX_THREADPOOL_VERSION "0.1.0"

namespace vix::threadpool
{
  /**
   * @brief Major version of the threadpool module.
   */
  inline constexpr int version_major = VIX_THREADPOOL_VERSION_MAJOR;

  /**
   * @brief Minor version of the threadpool module.
   */
  inline constexpr int version_minor = VIX_THREADPOOL_VERSION_MINOR;

  /**
   * @brief Patch version of the threadpool module.
   */
  inline constexpr int version_patch = VIX_THREADPOOL_VERSION_PATCH;

  /**
   * @brief Complete semantic version string of the threadpool module.
   */
  inline constexpr const char *version = VIX_THREADPOOL_VERSION;

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_VERSION_HPP
