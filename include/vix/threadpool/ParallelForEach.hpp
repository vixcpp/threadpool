/**
 *
 * @file ParallelForEach.hpp
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
#ifndef VIX_THREADPOOL_PARALLEL_FOR_EACH_HPP
#define VIX_THREADPOOL_PARALLEL_FOR_EACH_HPP

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>

namespace vix::threadpool
{
  /**
   * @brief Options used by parallel_for_each.
   *
   * ParallelForEachOptions controls how iterator ranges are chunked and how
   * chunk tasks are submitted to the pool.
   */
  struct ParallelForEachOptions
  {
    /**
     * @brief Number of elements processed by one submitted task.
     *
     * A value of 0 means the chunk size is selected automatically.
     */
    std::size_t chunk_size;

    /**
     * @brief Task options used for each submitted chunk.
     */
    TaskOptions task_options;

    /**
     * @brief Construct default parallel-for-each options.
     */
    ParallelForEachOptions() noexcept
        : chunk_size(0),
          task_options()
    {
    }

    /**
     * @brief Create options with a fixed chunk size.
     *
     * @param value Number of elements per chunk.
     * @return Configured options.
     */
    [[nodiscard]] static ParallelForEachOptions with_chunk_size(
        std::size_t value) noexcept
    {
      ParallelForEachOptions options;
      options.chunk_size = value;
      return options;
    }
  };

  /**
   * @brief Execute an iterator range in parallel.
   *
   * The callable is invoked once per element in the range [first, last).
   * Random-access iterators are split directly by index. Non-random-access
   * iterators are still supported, but chunk discovery is necessarily linear.
   *
   * If any chunk throws, the first exception is rethrown after all submitted
   * chunks finish.
   *
   * @tparam Iterator Iterator type.
   * @tparam Fn Callable type invocable as fn(*iterator).
   * @param pool Thread pool used for execution.
   * @param first First iterator.
   * @param last Last iterator.
   * @param fn Callable executed for each element.
   * @param options Parallel-for-each options.
   */
  template <class Iterator, class Fn>
  void parallel_for_each(
      ThreadPool &pool,
      Iterator first,
      Iterator last,
      Fn &&fn,
      ParallelForEachOptions options = ParallelForEachOptions{})
  {
    using Category =
        typename std::iterator_traits<Iterator>::iterator_category;

    using Function = std::decay_t<Fn>;

    const auto distance = std::distance(first, last);
    if (distance <= 0)
    {
      return;
    }

    const std::size_t total = static_cast<std::size_t>(distance);
    const std::size_t chunkSize =
        compute_parallel_chunk_size(
            total,
            pool.thread_count(),
            options.chunk_size);

    std::vector<Future<void>> futures;
    futures.reserve((total + chunkSize - 1) / chunkSize);

    auto sharedFn = std::make_shared<Function>(std::forward<Fn>(fn));

    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Category>)
    {
      for (std::size_t offset = 0; offset < total; offset += chunkSize)
      {
        const std::size_t remaining = total - offset;
        const std::size_t count = std::min(chunkSize, remaining);

        Iterator chunkFirst = first + static_cast<typename std::iterator_traits<Iterator>::difference_type>(offset);
        Iterator chunkLast = chunkFirst + static_cast<typename std::iterator_traits<Iterator>::difference_type>(count);

        futures.push_back(
            pool.submit(
                [sharedFn, chunkFirst, chunkLast]() mutable
                {
                  for (Iterator it = chunkFirst; it != chunkLast; ++it)
                  {
                    (*sharedFn)(*it);
                  }
                },
                options.task_options));
      }
    }
    else
    {
      Iterator chunkFirst = first;

      while (chunkFirst != last)
      {
        Iterator chunkLast = chunkFirst;
        std::size_t count = 0;

        while (chunkLast != last && count < chunkSize)
        {
          ++chunkLast;
          ++count;
        }

        futures.push_back(
            pool.submit(
                [sharedFn, chunkFirst, chunkLast]() mutable
                {
                  for (Iterator it = chunkFirst; it != chunkLast; ++it)
                  {
                    (*sharedFn)(*it);
                  }
                },
                options.task_options));

        chunkFirst = chunkLast;
      }
    }

    std::exception_ptr firstException = nullptr;

    for (auto &future : futures)
    {
      try
      {
        future.get();
      }
      catch (...)
      {
        if (!firstException)
        {
          firstException = std::current_exception();
        }
      }
    }

    if (firstException)
    {
      std::rethrow_exception(firstException);
    }
  }

  /**
   * @brief Execute all elements of a container in parallel.
   *
   * @tparam Container Container type.
   * @tparam Fn Callable type invocable as fn(element).
   * @param pool Thread pool used for execution.
   * @param container Container to iterate.
   * @param fn Callable executed for each element.
   * @param options Parallel-for-each options.
   */
  template <class Container, class Fn>
  void parallel_for_each(
      ThreadPool &pool,
      Container &container,
      Fn &&fn,
      ParallelForEachOptions options = ParallelForEachOptions{})
  {
    parallel_for_each(
        pool,
        std::begin(container),
        std::end(container),
        std::forward<Fn>(fn),
        std::move(options));
  }

  /**
   * @brief Execute an iterator range in parallel using a temporary pool.
   *
   * @tparam Iterator Iterator type.
   * @tparam Fn Callable type invocable as fn(*iterator).
   * @param first First iterator.
   * @param last Last iterator.
   * @param fn Callable executed for each element.
   * @param options Parallel-for-each options.
   */
  template <class Iterator, class Fn>
  void parallel_for_each(
      Iterator first,
      Iterator last,
      Fn &&fn,
      ParallelForEachOptions options = ParallelForEachOptions{})
  {
    ThreadPool pool;
    parallel_for_each(
        pool,
        first,
        last,
        std::forward<Fn>(fn),
        std::move(options));
  }

  /**
   * @brief Execute all elements of a container in parallel using a temporary pool.
   *
   * @tparam Container Container type.
   * @tparam Fn Callable type invocable as fn(element).
   * @param container Container to iterate.
   * @param fn Callable executed for each element.
   * @param options Parallel-for-each options.
   */
  template <class Container, class Fn>
  void parallel_for_each(
      Container &container,
      Fn &&fn,
      ParallelForEachOptions options = ParallelForEachOptions{})
  {
    ThreadPool pool;
    parallel_for_each(
        pool,
        container,
        std::forward<Fn>(fn),
        std::move(options));
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PARALLEL_FOR_EACH_HPP
