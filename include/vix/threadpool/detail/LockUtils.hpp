/**
 *
 * @file LockUtils.hpp
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
#ifndef VIX_THREADPOOL_DETAIL_LOCK_UTILS_HPP
#define VIX_THREADPOOL_DETAIL_LOCK_UTILS_HPP

#include <mutex>
#include <type_traits>
#include <utility>

namespace vix::threadpool::detail
{
  /**
   * @brief Execute a callable while holding a mutex lock.
   *
   * The mutex is locked using std::lock_guard for the whole duration of the
   * callable invocation.
   *
   * @tparam Mutex Mutex type.
   * @tparam Fn Callable type.
   * @param mutex Mutex to lock.
   * @param fn Callable executed while the mutex is locked.
   * @return Result produced by the callable.
   */
  template <class Mutex, class Fn>
  decltype(auto) with_lock(Mutex &mutex, Fn &&fn)
  {
    std::lock_guard<Mutex> lock(mutex);

    if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
    {
      std::forward<Fn>(fn)();
      return;
    }
    else
    {
      return std::forward<Fn>(fn)();
    }
  }

  /**
   * @brief Execute a callable while holding a unique lock.
   *
   * This helper is useful when the callable needs a lock object compatible
   * with condition_variable waiting or manual unlock operations.
   *
   * @tparam Mutex Mutex type.
   * @tparam Fn Callable type.
   * @param mutex Mutex to lock.
   * @param fn Callable receiving the unique lock by reference.
   * @return Result produced by the callable.
   */
  template <class Mutex, class Fn>
  decltype(auto) with_unique_lock(Mutex &mutex, Fn &&fn)
  {
    std::unique_lock<Mutex> lock(mutex);

    if constexpr (std::is_void_v<std::invoke_result_t<Fn, std::unique_lock<Mutex> &>>)
    {
      std::forward<Fn>(fn)(lock);
      return;
    }
    else
    {
      return std::forward<Fn>(fn)(lock);
    }
  }

  /**
   * @brief Try to lock a mutex and execute a callable only on success.
   *
   * The callable is executed only if the mutex can be locked immediately.
   *
   * @tparam Mutex Mutex type supporting try_lock().
   * @tparam Fn Callable type.
   * @param mutex Mutex to try-lock.
   * @param fn Callable executed when locking succeeds.
   * @return true if the callable was executed, false otherwise.
   */
  template <class Mutex, class Fn>
  [[nodiscard]] bool try_with_lock(Mutex &mutex, Fn &&fn)
  {
    std::unique_lock<Mutex> lock(mutex, std::try_to_lock);

    if (!lock.owns_lock())
    {
      return false;
    }

    std::forward<Fn>(fn)();
    return true;
  }

  /**
   * @brief Unlock a unique lock for the lifetime of this object.
   *
   * UnlockGuard temporarily releases a std::unique_lock and locks it again when
   * the guard is destroyed. It is useful when a callback must run without
   * holding an internal mutex.
   *
   * @tparam Mutex Mutex type used by the unique lock.
   */
  template <class Mutex>
  class UnlockGuard
  {
  public:
    /**
     * @brief Construct an unlock guard from a unique lock.
     *
     * @param lock Lock to temporarily release.
     */
    explicit UnlockGuard(std::unique_lock<Mutex> &lock) noexcept
        : lock_(lock),
          owns_(lock.owns_lock())
    {
      if (owns_)
      {
        lock_.unlock();
      }
    }

    /**
     * @brief Restore the lock if it was owned at construction.
     */
    ~UnlockGuard() noexcept
    {
      if (owns_)
      {
        lock_.lock();
      }
    }

    UnlockGuard(const UnlockGuard &) = delete;
    UnlockGuard &operator=(const UnlockGuard &) = delete;

  private:
    /**
     * @brief Lock temporarily released by this guard.
     */
    std::unique_lock<Mutex> &lock_;

    /**
     * @brief Whether the lock was owned at construction.
     */
    bool owns_;
  };

} // namespace vix::threadpool::detail

#endif // VIX_THREADPOOL_DETAIL_LOCK_UTILS_HPP
