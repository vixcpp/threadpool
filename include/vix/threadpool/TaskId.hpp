/**
 *
 * @file TaskId.hpp
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
#ifndef VIX_THREADPOOL_TASK_ID_HPP
#define VIX_THREADPOOL_TASK_ID_HPP

#include <cstdint>

namespace vix::threadpool
{
  /**
   * @brief Unique identifier type for tasks submitted to the thread pool.
   *
   * TaskId is used to identify a logical unit of work inside the threadpool
   * module. A value of 0 is reserved for an invalid or unassigned task.
   */
  using TaskId = std::uint64_t;

  /**
   * @brief Invalid task identifier.
   *
   * This value represents the absence of a valid task id.
   */
  inline constexpr TaskId invalid_task_id = 0;

  /**
   * @brief Check whether a task id is valid.
   *
   * @param id Task identifier to inspect.
   * @return true if the id is valid, false otherwise.
   */
  [[nodiscard]] inline constexpr bool is_valid_task_id(TaskId id) noexcept
  {
    return id != invalid_task_id;
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_ID_HPP
