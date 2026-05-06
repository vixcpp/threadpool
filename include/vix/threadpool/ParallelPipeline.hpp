/**
 *
 * @file ParallelPipeline.hpp
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
#ifndef VIX_THREADPOOL_PARALLEL_PIPELINE_HPP
#define VIX_THREADPOOL_PARALLEL_PIPELINE_HPP

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <vix/threadpool/Future.hpp>
#include <vix/threadpool/TaskOptions.hpp>
#include <vix/threadpool/ThreadPool.hpp>

namespace vix::threadpool
{
  /**
   * @brief Options used by parallel_pipeline.
   *
   * ParallelPipelineOptions controls how pipeline stages are submitted to the
   * thread pool.
   */
  struct ParallelPipelineOptions
  {
    /**
     * @brief Task options used for each submitted stage.
     */
    TaskOptions task_options;

    /**
     * @brief Construct default pipeline options.
     */
    ParallelPipelineOptions() noexcept
        : task_options()
    {
    }
  };

  /**
   * @brief Execute independent pipeline stages in parallel.
   *
   * Each stage is a callable with signature compatible with void().
   * Stages are submitted at the same time and then waited for.
   *
   * This helper is intentionally simple:
   * - stages are independent
   * - stage order is not guaranteed
   * - the first thrown exception is rethrown after all stages finish
   *
   * @tparam Stages Stage callable types.
   * @param pool Thread pool used for execution.
   * @param options Pipeline options.
   * @param stages Stage callables.
   */
  template <class... Stages>
  void parallel_pipeline(
      ThreadPool &pool,
      ParallelPipelineOptions options,
      Stages &&...stages)
  {
    if constexpr (sizeof...(Stages) == 0)
    {
      return;
    }
    else
    {
      std::vector<Future<void>> futures;
      futures.reserve(sizeof...(Stages));

      (
          futures.push_back(
              pool.submit(
                  [stage = std::decay_t<Stages>(std::forward<Stages>(stages))]() mutable
                  {
                    stage();
                  },
                  options.task_options)),
          ...);

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
  }

  /**
   * @brief Execute independent pipeline stages in parallel.
   *
   * Uses default pipeline options.
   *
   * @tparam Stages Stage callable types.
   * @param pool Thread pool used for execution.
   * @param stages Stage callables.
   */
  template <class... Stages>
  void parallel_pipeline(
      ThreadPool &pool,
      Stages &&...stages)
  {
    parallel_pipeline(
        pool,
        ParallelPipelineOptions{},
        std::forward<Stages>(stages)...);
  }

  /**
   * @brief Execute independent pipeline stages using a temporary pool.
   *
   * @tparam Stages Stage callable types.
   * @param options Pipeline options.
   * @param stages Stage callables.
   */
  template <class... Stages>
  void parallel_pipeline(
      ParallelPipelineOptions options,
      Stages &&...stages)
  {
    ThreadPool pool;

    parallel_pipeline(
        pool,
        std::move(options),
        std::forward<Stages>(stages)...);
  }

  /**
   * @brief Execute independent pipeline stages using a temporary pool.
   *
   * Uses default pipeline options.
   *
   * @tparam Stages Stage callable types.
   * @param stages Stage callables.
   */
  template <class... Stages>
  void parallel_pipeline(Stages &&...stages)
  {
    ThreadPool pool;

    parallel_pipeline(
        pool,
        ParallelPipelineOptions{},
        std::forward<Stages>(stages)...);
  }

  /**
   * @brief Small reusable pipeline builder.
   *
   * Pipeline stores independent void stages and runs them in parallel through a
   * ThreadPool.
   *
   * This class is useful when stages are assembled progressively before
   * execution.
   */
  class Pipeline
  {
  public:
    /**
     * @brief Stage callable type.
     */
    using Stage = std::function<void()>;

    /**
     * @brief Construct an empty pipeline.
     */
    Pipeline()
        : stages_(),
          options_()
    {
    }

    /**
     * @brief Construct a pipeline with options.
     *
     * @param options Pipeline options.
     */
    explicit Pipeline(ParallelPipelineOptions options)
        : stages_(),
          options_(std::move(options))
    {
    }

    /**
     * @brief Add one stage to the pipeline.
     *
     * @tparam Fn Callable type compatible with void().
     * @param fn Stage callable.
     * @return Reference to this pipeline.
     */
    template <class Fn>
    Pipeline &add(Fn &&fn)
    {
      stages_.emplace_back(std::forward<Fn>(fn));
      return *this;
    }

    /**
     * @brief Remove all stages.
     */
    void clear()
    {
      stages_.clear();
    }

    /**
     * @brief Return the number of registered stages.
     *
     * @return Stage count.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return stages_.size();
    }

    /**
     * @brief Check whether no stage is registered.
     *
     * @return true if empty.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return stages_.empty();
    }

    /**
     * @brief Return pipeline options.
     *
     * @return Pipeline options.
     */
    [[nodiscard]] const ParallelPipelineOptions &options() const noexcept
    {
      return options_;
    }

    /**
     * @brief Set pipeline options.
     *
     * @param options New pipeline options.
     */
    void set_options(ParallelPipelineOptions options)
    {
      options_ = std::move(options);
    }

    /**
     * @brief Run all registered stages in parallel.
     *
     * @param pool Thread pool used for execution.
     */
    void run(ThreadPool &pool) const
    {
      std::vector<Future<void>> futures;
      futures.reserve(stages_.size());

      for (const auto &stage : stages_)
      {
        futures.push_back(
            pool.submit(
                [stage]()
                {
                  stage();
                },
                options_.task_options));
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
     * @brief Run all registered stages using a temporary pool.
     */
    void run() const
    {
      ThreadPool pool;
      run(pool);
    }

  private:
    /**
     * @brief Registered pipeline stages.
     */
    std::vector<Stage> stages_;

    /**
     * @brief Pipeline execution options.
     */
    ParallelPipelineOptions options_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_PARALLEL_PIPELINE_HPP
