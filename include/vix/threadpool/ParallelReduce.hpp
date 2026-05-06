/**
 *
 * @file ParallelReduce.hpp
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
#ifndef VIX_THREADPOOL_PARALLEL_REDUCE_HPP
#define VIX_THREADPOOL_PARALLEL_REDUCE_HPP

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
   * @brief Options used by parallel_reduce.
   *
   * ParallelReduceOptions controls how input ranges are split into chunks and
   * how chunk tasks are submitted to the pool.
   */
  struct ParallelReduceOptions
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
     * @brief Construct default parallel-reduce options.
     */
    ParallelReduceOptions() noexcept
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
    [[nodiscard]] static ParallelReduceOptions with_chunk_size(
        std::size_t value) noexcept
    {
      ParallelReduceOptions options;
      options.chunk_size = value;
      return options;
    }
  };

  /**
   * @brief Reduce an iterator range in parallel.
   *
   * Each chunk starts from the provided initial value, reduces its local range,
   * and then all partial values are reduced again on the caller thread.
   *
   * @tparam Iterator Iterator type.
   * @tparam T Accumulator type.
   * @tparam ReduceFn Callable type invocable as reduce(acc, value).
   * @param pool Thread pool used for execution.
   * @param first First iterator.
   * @param last Last iterator.
   * @param initial Initial accumulator value.
   * @param reduce Reduction callable.
   * @param options Parallel-reduce options.
   * @return Final reduced value.
   */
  template <class Iterator, class T, class ReduceFn>
  [[nodiscard]] T parallel_reduce(
      ThreadPool &pool,
      Iterator first,
      Iterator last,
      T initial,
      ReduceFn &&reduce,
      ParallelReduceOptions options = ParallelReduceOptions{})
  {
    using Category =
        typename std::iterator_traits<Iterator>::iterator_category;

    using Reducer = std::decay_t<ReduceFn>;

    const auto distance = std::distance(first, last);
    if (distance <= 0)
    {
      return initial;
    }

    const std::size_t total = static_cast<std::size_t>(distance);
    const std::size_t chunkSize =
        compute_parallel_chunk_size(
            total,
            pool.thread_count(),
            options.chunk_size);

    std::vector<Future<T>> futures;
    futures.reserve((total + chunkSize - 1) / chunkSize);

    auto sharedReduce =
        std::make_shared<Reducer>(std::forward<ReduceFn>(reduce));

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
                [sharedReduce, chunkFirst, count, initial]() mutable
                {
                  T local = initial;
                  Iterator it = chunkFirst;

                  for (std::size_t i = 0; i < count; ++i, ++it)
                  {
                    local = (*sharedReduce)(std::move(local), *it);
                  }

                  return local;
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
                [sharedReduce, chunkFirst, chunkLast, initial]() mutable
                {
                  T local = initial;

                  for (Iterator it = chunkFirst; it != chunkLast; ++it)
                  {
                    local = (*sharedReduce)(std::move(local), *it);
                  }

                  return local;
                },
                options.task_options));

        chunkFirst = chunkLast;
      }
    }

    std::exception_ptr firstException = nullptr;
    std::vector<T> partials;
    partials.reserve(futures.size());

    for (auto &future : futures)
    {
      try
      {
        partials.push_back(future.get());
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

    T result = initial;

    for (auto &partial : partials)
    {
      result = (*sharedReduce)(std::move(result), partial);
    }

    return result;
  }

  /**
   * @brief Reduce a container in parallel.
   *
   * @tparam Container Container type.
   * @tparam T Accumulator type.
   * @tparam ReduceFn Callable type invocable as reduce(acc, value).
   * @param pool Thread pool used for execution.
   * @param container Input container.
   * @param initial Initial accumulator value.
   * @param reduce Reduction callable.
   * @param options Parallel-reduce options.
   * @return Final reduced value.
   */
  template <class Container, class T, class ReduceFn>
  [[nodiscard]] T parallel_reduce(
      ThreadPool &pool,
      Container &container,
      T initial,
      ReduceFn &&reduce,
      ParallelReduceOptions options = ParallelReduceOptions{})
  {
    return parallel_reduce(
        pool,
        std::begin(container),
        std::end(container),
        std::move(initial),
        std::forward<ReduceFn>(reduce),
        std::move(options));
  }

  /**
   * @brief Reduce an iterator range in parallel using a temporary pool.
   *
   * @tparam Iterator Iterator type.
   * @tparam T Accumulator type.
   * @tparam ReduceFn Callable type invocable as reduce(acc, value).
   * @param first First iterator.
   * @param last Last iterator.
   * @param initial Initial accumulator value.
   * @param reduce Reduction callable.
   * @param options Parallel-reduce options.
   * @return Final reduced value.
   */
  template <class Iterator, class T, class ReduceFn>
  [[nodiscard]] T parallel_reduce(
      Iterator first,
      Iterator last,
      T initial,
      ReduceFn &&reduce,
      ParallelReduceOptions options = ParallelReduceOptions{})
  {
    ThreadPool pool;

    return parallel_reduce(
        pool,
        first,
        last,
        std::move(initial),
        std::forward<ReduceFn>(reduce),
        std::move(options));
  }

  /**
   * @brief Reduce a container in parallel using a temporary pool.
   *
   * @tparam Container Container type.
   * @tparam T Accumulator type.
   * @tparam ReduceFn Callable type invocable as reduce(acc, value).
   * @param container Input container.
   * @param initial Initial accumulator value.
   * @param reduce Reduction callable.
   * @param options Parallel-reduce options.
   * @return Final reduced value.
   */
  template <class Container, class T, class ReduceFn>
  [[nodiscard]] T parallel_reduce(
      Container &container,
      T initial,
      ReduceFn &&reduce,
      ParallelReduceOptions options = ParallelReduceOptions{})
  {
    ThreadPool pool;

    return parallel_reduce(
        pool,
        container,
        std::move(initial),
        std::forward<ReduceFn>(reduce),
        std::move(options));
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PARALLEL_REDUCE_HPP
