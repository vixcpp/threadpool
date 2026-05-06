/**
 *
 * @file Parallel.hpp
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
#ifndef VIX_THREADPOOL_PARALLEL_HPP
#define VIX_THREADPOOL_PARALLEL_HPP

#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/ParallelForEach.hpp>
#include <vix/threadpool/ParallelMap.hpp>
#include <vix/threadpool/ParallelPipeline.hpp>
#include <vix/threadpool/ParallelReduce.hpp>

namespace vix::threadpool
{
  /**
   * @brief Convenience namespace for high-level parallel algorithms.
   *
   * The functions in this namespace forward to the public threadpool parallel
   * APIs while giving users a compact and readable entry point:
   *
   * @code
   * vix::threadpool::parallel::for_range(pool, 0, 100, fn);
   * vix::threadpool::parallel::for_each(pool, items, fn);
   * vix::threadpool::parallel::map(pool, items, fn);
   * vix::threadpool::parallel::reduce(pool, items, 0, fn);
   * @endcode
   */
  namespace parallel
  {
    /**
     * @brief Execute a numeric range in parallel.
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
    void for_range(
        ThreadPool &pool,
        Index first,
        Index last,
        Fn &&fn,
        ParallelForOptions options = ParallelForOptions{})
    {
      parallel_for(
          pool,
          first,
          last,
          std::forward<Fn>(fn),
          std::move(options));
    }

    /**
     * @brief Execute a numeric range in parallel using a temporary pool.
     *
     * @tparam Index Integral index type.
     * @tparam Fn Callable type invocable as fn(Index).
     * @param first First index, inclusive.
     * @param last Last index, exclusive.
     * @param fn Callable executed for each index.
     * @param options Parallel-for options.
     */
    template <class Index, class Fn>
    void for_range(
        Index first,
        Index last,
        Fn &&fn,
        ParallelForOptions options = ParallelForOptions{})
    {
      parallel_for(
          first,
          last,
          std::forward<Fn>(fn),
          std::move(options));
    }

    /**
     * @brief Execute an iterator range in parallel.
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
    void for_each(
        ThreadPool &pool,
        Iterator first,
        Iterator last,
        Fn &&fn,
        ParallelForEachOptions options = ParallelForEachOptions{})
    {
      parallel_for_each(
          pool,
          first,
          last,
          std::forward<Fn>(fn),
          std::move(options));
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
    void for_each(
        ThreadPool &pool,
        Container &container,
        Fn &&fn,
        ParallelForEachOptions options = ParallelForEachOptions{})
    {
      parallel_for_each(
          pool,
          container,
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
    void for_each(
        Iterator first,
        Iterator last,
        Fn &&fn,
        ParallelForEachOptions options = ParallelForEachOptions{})
    {
      parallel_for_each(
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
    void for_each(
        Container &container,
        Fn &&fn,
        ParallelForEachOptions options = ParallelForEachOptions{})
    {
      parallel_for_each(
          container,
          std::forward<Fn>(fn),
          std::move(options));
    }

    /**
     * @brief Map an iterator range in parallel.
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
    [[nodiscard]] auto map(
        ThreadPool &pool,
        Iterator first,
        Iterator last,
        Fn &&fn,
        ParallelMapOptions options = ParallelMapOptions{})
        -> decltype(parallel_map(
            pool,
            first,
            last,
            std::forward<Fn>(fn),
            std::move(options)))
    {
      return parallel_map(
          pool,
          first,
          last,
          std::forward<Fn>(fn),
          std::move(options));
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
    [[nodiscard]] auto map(
        ThreadPool &pool,
        Container &container,
        Fn &&fn,
        ParallelMapOptions options = ParallelMapOptions{})
        -> decltype(parallel_map(
            pool,
            container,
            std::forward<Fn>(fn),
            std::move(options)))
    {
      return parallel_map(
          pool,
          container,
          std::forward<Fn>(fn),
          std::move(options));
    }

    /**
     * @brief Reduce an iterator range in parallel.
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
    [[nodiscard]] T reduce(
        ThreadPool &pool,
        Iterator first,
        Iterator last,
        T initial,
        ReduceFn &&reduce,
        ParallelReduceOptions options = ParallelReduceOptions{})
    {
      return parallel_reduce(
          pool,
          first,
          last,
          std::move(initial),
          std::forward<ReduceFn>(reduce),
          std::move(options));
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
    [[nodiscard]] T reduce(
        ThreadPool &pool,
        Container &container,
        T initial,
        ReduceFn &&reduce,
        ParallelReduceOptions options = ParallelReduceOptions{})
    {
      return parallel_reduce(
          pool,
          container,
          std::move(initial),
          std::forward<ReduceFn>(reduce),
          std::move(options));
    }

    /**
     * @brief Run independent stages in parallel.
     *
     * @tparam Stages Stage callable types.
     * @param pool Thread pool used for execution.
     * @param stages Stage callables.
     */
    template <class... Stages>
    void pipeline(
        ThreadPool &pool,
        Stages &&...stages)
    {
      parallel_pipeline(
          pool,
          std::forward<Stages>(stages)...);
    }

    /**
     * @brief Run independent stages in parallel with explicit options.
     *
     * @tparam Stages Stage callable types.
     * @param pool Thread pool used for execution.
     * @param options Pipeline options.
     * @param stages Stage callables.
     */
    template <class... Stages>
    void pipeline(
        ThreadPool &pool,
        ParallelPipelineOptions options,
        Stages &&...stages)
    {
      parallel_pipeline(
          pool,
          std::move(options),
          std::forward<Stages>(stages)...);
    }

  } // namespace parallel

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PARALLEL_HPP
