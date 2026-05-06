/**
 *
 * @file WorkerId.hpp
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
#ifndef VIX_THREADPOOL_WORKER_ID_HPP
#define VIX_THREADPOOL_WORKER_ID_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Unique identifier type for worker threads owned by a thread pool.
   *
   * WorkerId identifies one worker inside a thread pool instance. A value of 0
   * is reserved for an invalid or unassigned worker.
   */
  using WorkerId = std::uint32_t;

  /**
   * @brief Invalid worker identifier.
   *
   * This value represents the absence of a valid worker id.
   */
  inline constexpr WorkerId invalid_worker_id = 0;

  /**
   * @brief Check whether a worker id is valid.
   *
   * @param id Worker identifier to inspect.
   * @return true if the id is valid, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_valid_worker_id(WorkerId id) noexcept
  {
    return id != invalid_worker_id;
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_WORKER_ID_HPP
