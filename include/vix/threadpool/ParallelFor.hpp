/**
 *
 * @file ParallelFor.hpp
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
#ifndef VIX_THREADPOOL_PARALLEL_FOR_HPP
#define VIX_THREADPOOL_PARALLEL_FOR_HPP

#include <algorithm>
#include <cstddef>
#include <exception>
#include <future>
#include <type_traits>
#include <utility>
#include <vector>

#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>

namespace vix::threadpool
{
  /**
   * @brief Options used by parallel_for.
   *
   * ParallelForOptions controls how an index range is split into chunks and how
   * each chunk is submitted to the thread pool.
   */
  struct ParallelForOptions
  {
    /**
     * @brief Number of indices processed by one submitted task.
     *
     * A value of 0 means the chunk size is selected automatically.
     */
    std::size_t chunk_size;

    /**
     * @brief Task options used for each submitted chunk.
     */
    TaskOptions task_options;

    /**
     * @brief Construct default parallel-for options.
     */
    ParallelForOptions() noexcept
        : chunk_size(0),
          task_options()
    {
    }

    /**
     * @brief Create options with a fixed chunk size.
     *
     * @param value Number of indices per chunk.
     * @return Configured options.
     */
    [[nodiscard]] static ParallelForOptions with_chunk_size(std::size_t value) noexcept
    {
      ParallelForOptions options;
      options.chunk_size = value;
      return options;
    }
  };

  /**
   * @brief Compute a safe chunk size for a parallel-for range.
   *
   * @param total Number of items in the range.
   * @param workerCount Number of worker threads available.
   * @param requested Requested chunk size. Zero means automatic.
   * @return Normalized chunk size.
   */
  [[nodiscard]] inline std::size_t compute_parallel_chunk_size(
      std::size_t total,
      std::size_t workerCount,
      std::size_t requested) noexcept
  {
    if (total == 0)
    {
      return 1;
    }

    if (requested > 0)
    {
      return requested;
    }

    if (workerCount == 0)
    {
      workerCount = 1;
    }

    const std::size_t targetChunks = workerCount * 4;
    const std::size_t chunk = (total + targetChunks - 1) / targetChunks;

    return chunk == 0 ? 1 : chunk;
  }

  /**
   * @brief Execute a numeric index range in parallel.
   *
   * The callable is invoked once per index in the half-open range
   * [first, last). Work is split into chunks and each chunk is submitted to the
   * provided pool.
   *
   * If any chunk throws, the first exception is rethrown after all submitted
   * chunks finish.
   *
   * @tparam Index Integral index type.
   * @tparam Fn Callable type invocable as fn(Index).
   * @param pool Thread pool used for execution.
   * @param first First index, inclusive.
   * @param last Last index, exclusive.
   * @param fn Callable executed for each index.
   * @param options Parallel-for options.
   */
  template <class Index, class Fn>
  void parallel_for(
      ThreadPool &pool,
      Index first,
      Index last,
      Fn &&fn,
      ParallelForOptions options = ParallelForOptions{})
  {
    static_assert(std::is_integral_v<Index>,
                  "parallel_for requires an integral index type");

    if (last <= first)
    {
      return;
    }

    using Function = std::decay_t<Fn>;

    const auto total =
        static_cast<std::size_t>(last - first);

    const std::size_t chunkSize =
        compute_parallel_chunk_size(
            total,
            pool.thread_count(),
            options.chunk_size);

    std::vector<Future<void>> futures;
    futures.reserve((total + chunkSize - 1) / chunkSize);

    auto sharedFn = std::make_shared<Function>(std::forward<Fn>(fn));

    for (std::size_t offset = 0; offset < total; offset += chunkSize)
    {
      const Index chunkFirst =
          static_cast<Index>(first + static_cast<Index>(offset));

      const std::size_t remaining = total - offset;
      const std::size_t count = std::min(chunkSize, remaining);

      const Index chunkLast =
          static_cast<Index>(chunkFirst + static_cast<Index>(count));

      futures.push_back(
          pool.submit(
              [sharedFn, chunkFirst, chunkLast]()
              {
                for (Index i = chunkFirst; i < chunkLast; ++i)
                {
                  (*sharedFn)(i);
                }
              },
              options.task_options));
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
   * @brief Execute a numeric index range in parallel using a temporary pool.
   *
   * @tparam Index Integral index type.
   * @tparam Fn Callable type invocable as fn(Index).
   * @param first First index, inclusive.
   * @param last Last index, exclusive.
   * @param fn Callable executed for each index.
   * @param options Parallel-for options.
   */
  template <class Index, class Fn>
  void parallel_for(
      Index first,
      Index last,
      Fn &&fn,
      ParallelForOptions options = ParallelForOptions{})
  {
    ThreadPool pool;
    parallel_for(pool, first, last, std::forward<Fn>(fn), std::move(options));
  }

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PARALLEL_FOR_HPP
