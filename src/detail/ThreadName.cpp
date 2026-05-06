/**
 *
 * @file ThreadName.cpp
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
#include <vix/threadpool/detail/ThreadName.hpp>

#include <algorithm>
#include <string>

#if defined(__linux__)
#include <pthread.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

namespace vix::threadpool::detail
{
  std::string normalize_thread_name(const std::string &name)
  {
    if (name.empty())
    {
      return {};
    }

    if (name.size() <= max_thread_name_length)
    {
      return name;
    }

    return name.substr(0, max_thread_name_length);
  }

  std::string make_worker_thread_name(
      const std::string &prefix,
      std::size_t index)
  {
    std::string base = prefix.empty() ? "vix-tp" : prefix;
    std::string suffix = "-" + std::to_string(index);

    if (base.size() + suffix.size() <= max_thread_name_length)
    {
      return normalize_thread_name(base + suffix);
    }

    const std::size_t maxPrefixSize =
        max_thread_name_length > suffix.size()
            ? max_thread_name_length - suffix.size()
            : 0;

    if (maxPrefixSize == 0)
    {
      return normalize_thread_name(suffix);
    }

    base = base.substr(0, maxPrefixSize);
    return normalize_thread_name(base + suffix);
  }

  void set_current_thread_name(const std::string &name) noexcept
  {
    const std::string normalized = normalize_thread_name(name);

    if (normalized.empty())
    {
      return;
    }

#if defined(__linux__)
    (void)pthread_setname_np(pthread_self(), normalized.c_str());
#elif defined(__APPLE__)
    (void)pthread_setname_np(normalized.c_str());
#else
    (void)normalized;
#endif
  }

  void set_thread_name(std::thread &thread, const std::string &name) noexcept
  {
    const std::string normalized = normalize_thread_name(name);

    if (normalized.empty() || !thread.joinable())
    {
      return;
    }

#if defined(__linux__)
    (void)pthread_setname_np(thread.native_handle(), normalized.c_str());
#else
    (void)thread;
    (void)normalized;
#endif
  }

} // namespace vix::threadpool::detail
