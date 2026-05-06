/**
 *
 * @file ParallelMap.hpp
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
#ifndef VIX_THREADPOOL_PARALLEL_MAP_HPP
#define VIX_THREADPOOL_PARALLEL_MAP_HPP

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
   * @brief Options used by parallel_map.
   *
   * ParallelMapOptions controls how input ranges are split into chunks and how
   * chunk tasks are submitted to the pool.
   */
  struct ParallelMapOptions
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
     * @brief Construct default parallel-map options.
     */
    ParallelMapOptions() noexcept
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
    [[nodiscard]] static ParallelMapOptions with_chunk_size(
        std::size_t value) noexcept
    {
      ParallelMapOptions options;
      options.chunk_size = value;
      return options;
    }
  };

  /**
   * @brief Map an iterator range in parallel and return a vector of results.
   *
   * The callable is invoked once per input element. Output order matches input
   * order.
   *
   * @tparam Iterator Iterator type.
   * @tparam Fn Callable type invocable as fn(*iterator).
   * @param pool Thread pool used for execution.
   * @param first First iterator.
   * @param last Last iterator.
   * @param fn Mapping callable.
   * @param options Parallel-map options.
   * @return Vector containing mapped values in input order.
   */
  template <class Iterator, class Fn>
  [[nodiscard]] auto parallel_map(
      ThreadPool &pool,
      Iterator first,
      Iterator last,
      Fn &&fn,
      ParallelMapOptions options = ParallelMapOptions{})
      -> std::vector<std::invoke_result_t<
          std::decay_t<Fn> &,
          decltype(*std::declval<Iterator &>())>>
  {
    using Category =
        typename std::iterator_traits<Iterator>::iterator_category;

    using Function = std::decay_t<Fn>;
    using InputRef = decltype(*std::declval<Iterator &>());
    using Result = std::invoke_result_t<Function &, InputRef>;

    const auto distance = std::distance(first, last);
    if (distance <= 0)
    {
      return {};
    }

    const std::size_t total = static_cast<std::size_t>(distance);
    const std::size_t chunkSize =
        compute_parallel_chunk_size(
            total,
            pool.thread_count(),
            options.chunk_size);

    std::vector<Result> output(total);
    std::vector<Future<void>> futures;
    futures.reserve((total + chunkSize - 1) / chunkSize);

    auto sharedFn = std::make_shared<Function>(std::forward<Fn>(fn));

    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Category>)
    {
      for (std::size_t offset = 0; offset < total; offset += chunkSize)
      {
        const std::size_t remaining = total - offset;
        const std::size_t count = std::min(chunkSize, remaining);

        Iterator chunkFirst =
            first +
            static_cast<typename std::iterator_traits<Iterator>::difference_type>(offset);

        futures.push_back(
            pool.submit(
                [sharedFn, chunkFirst, count, offset, &output]() mutable
                {
                  Iterator it = chunkFirst;

                  for (std::size_t i = 0; i < count; ++i, ++it)
                  {
                    output[offset + i] = (*sharedFn)(*it);
                  }
                },
                options.task_options));
      }
    }
    else
    {
      Iterator chunkFirst = first;
      std::size_t offset = 0;

      while (chunkFirst != last)
      {
        Iterator chunkLast = chunkFirst;
        std::size_t count = 0;

        while (chunkLast != last && count < chunkSize)
        {
          ++chunkLast;
          ++count;
        }

        const std::size_t chunkOffset = offset;

        futures.push_back(
            pool.submit(
                [sharedFn, chunkFirst, chunkLast, chunkOffset, &output]() mutable
                {
                  std::size_t i = 0;

                  for (Iterator it = chunkFirst; it != chunkLast; ++it, ++i)
                  {
                    output[chunkOffset + i] = (*sharedFn)(*it);
                  }
                },
                options.task_options));

        offset += count;
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

    return output;
  }

  /**
   * @brief Map all elements of a container in parallel.
   *
   * @tparam Container Container type.
   * @tparam Fn Callable type invocable as fn(element).
   * @param pool Thread pool used for execution.
   * @param container Input container.
   * @param fn Mapping callable.
   * @param options Parallel-map options.
   * @return Vector containing mapped values in input order.
   */
  template <class Container, class Fn>
  [[nodiscard]] auto parallel_map(
      ThreadPool &pool,
      Container &container,
      Fn &&fn,
      ParallelMapOptions options = ParallelMapOptions{})
      -> decltype(parallel_map(
          pool,
          std::begin(container),
          std::end(container),
          std::forward<Fn>(fn),
          std::move(options)))
  {
    return parallel_map(
        pool,
        std::begin(container),
        std::end(container),
        std::forward<Fn>(fn),
        std::move(options));
  }

  /**
   * @brief Map an iterator range in parallel using a temporary pool.
   *
   * @tparam Iterator Iterator type.
   * @tparam Fn Callable type invocable as fn(*iterator).
   * @param first First iterator.
   * @param last Last iterator.
   * @param fn Mapping callable.
   * @param options Parallel-map options.
   * @return Vector containing mapped values in input order.
   */
  template <class Iterator, class Fn>
  [[nodiscard]] auto parallel_map(
      Iterator first,
      Iterator last,
      Fn &&fn,
      ParallelMapOptions options = ParallelMapOptions{})
      -> decltype(parallel_map(
          std::declval<ThreadPool &>(),
          first,
          last,
          std::forward<Fn>(fn),
          std::move(options)))
  {
    ThreadPool pool;

    return parallel_map(
        pool,
        first,
        last,
        std::forward<Fn>(fn),
        std::move(options));
  }

  /**
   * @brief Map all elements of a container in parallel using a temporary pool.
   *
   * @tparam Container Container type.
   * @tparam Fn Callable type invocable as fn(element).
   * @param container Input container.
   * @param fn Mapping callable.
   * @param options Parallel-map options.
   * @return Vector containing mapped values in input order.
   */
  template <class Container, class Fn>
  [[nodiscard]] auto parallel_map(
      Container &container,
      Fn &&fn,
      ParallelMapOptions options = ParallelMapOptions{})
      -> decltype(parallel_map(
          std::declval<ThreadPool &>(),
          container,
          std::forward<Fn>(fn),
          std::move(options)))
  {
    ThreadPool pool;

    return parallel_map(
        pool,
        container,
        std::forward<Fn>(fn),
        std::move(options));
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PARALLEL_MAP_HPP
