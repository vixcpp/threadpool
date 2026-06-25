/**
 *
 * @file Scheduler.hpp
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
#ifndef VIX_THREADPOOL_SCHEDULER_HPP
#define VIX_THREADPOOL_SCHEDULER_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <vix/threadpool/RejectionPolicy.hpp>
#include <vix/threadpool/SchedulingPolicy.hpp>
#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/ThreadPoolMetrics.hpp>
#include <vix/threadpool/ThreadPoolStats.hpp>
#include <vix/threadpool/Worker.hpp>
#include <vix/threadpool/WorkerId.hpp>
#include <vix/threadpool/detail/ThreadName.hpp>

namespace vix::threadpool
{
  /**
   * @brief Configuration used to construct a Scheduler.
   *
   * SchedulerConfig controls worker creation, local queue capacity,
   * distribution strategy, rejection behavior, and shutdown draining.
   */
  struct SchedulerConfig
  {
    /**
     * @brief Number of workers owned by the scheduler.
     *
     * A value of zero is normalized to one.
     */
    std::size_t worker_count;

    /**
     * @brief Maximum number of queued tasks per worker.
     *
     * A value of zero means each worker queue is unbounded.
     */
    std::size_t max_queue_size_per_worker;

    /**
     * @brief Policy used to select the destination worker.
     */
    SchedulingPolicy scheduling_policy;

    /**
     * @brief Policy used when a task cannot be accepted.
     */
    RejectionPolicy rejection_policy;

    /**
     * @brief Whether workers should drain queued tasks during shutdown.
     */
    bool drain_on_stop;

    /**
     * @brief Prefix used to name worker threads.
     */
    std::string worker_name_prefix;

    /**
     * @brief Construct a default scheduler configuration.
     */
    SchedulerConfig()
        : worker_count(1),
          max_queue_size_per_worker(0),
          scheduling_policy(default_scheduling_policy()),
          rejection_policy(default_rejection_policy()),
          drain_on_stop(true),
          worker_name_prefix("vix-tp")
    {
    }

    /**
     * @brief Return a normalized copy of this configuration.
     *
     * @return Normalized scheduler configuration.
     */
    [[nodiscard]] SchedulerConfig normalized() const
    {
      SchedulerConfig out = *this;

      if (out.worker_count == 0)
      {
        out.worker_count = 1;
      }

      if (out.worker_name_prefix.empty())
      {
        out.worker_name_prefix = "vix-tp";
      }

      return out;
    }
  };

  /**
   * @brief Scheduler responsible for distributing tasks across workers.
   *
   * Scheduler owns a fixed set of Worker objects and exposes the low-level
   * scheduling layer used by ThreadPool.
   *
   * Responsibilities:
   * - create and start workers
   * - stop and join workers
   * - choose a worker for each task
   * - apply rejection policy
   * - collect aggregate metrics and stats
   */
  class Scheduler
  {
  public:
    /**
     * @brief Construct a scheduler from configuration.
     *
     * Workers are created immediately but not started until start() is called.
     *
     * @param config Scheduler configuration.
     */
    explicit Scheduler(SchedulerConfig config = SchedulerConfig{})
        : config_(std::move(config).normalized()),
          workers_(),
          next_worker_(0),
          running_(false),
          stopping_(false),
          submitted_tasks_(0),
          accepted_tasks_(0),
          rejected_tasks_(0),
          caller_runs_tasks_(0)
    {
      workers_.reserve(config_.worker_count);

      for (std::size_t i = 0; i < config_.worker_count; ++i)
      {
        const WorkerId id = static_cast<WorkerId>(i + 1u);
        const std::string name =
            detail::make_worker_thread_name(config_.worker_name_prefix, i);

        auto worker = std::make_unique<Worker>(
            id,
            i,
            config_.max_queue_size_per_worker,
            name);

        worker->set_drain_on_stop(config_.drain_on_stop);

        workers_.push_back(std::move(worker));
      }
    }

    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    Scheduler(Scheduler &&) = delete;
    Scheduler &operator=(Scheduler &&) = delete;

    /**
     * @brief Stop and join workers on destruction.
     */
    ~Scheduler() noexcept
    {
      stop();
      join();
    }

    /**
     * @brief Start all workers.
     *
     * Calling start() more than once has no effect.
     *
     * @return true if the scheduler transitioned to running.
     */
    [[nodiscard]] bool start()
    {
      bool expected = false;
      if (!running_.compare_exchange_strong(
              expected,
              true,
              std::memory_order_acq_rel,
              std::memory_order_acquire))
      {
        return false;
      }

      stopping_.store(false, std::memory_order_release);

      bool ok = true;

      for (auto &worker : workers_)
      {
        if (!worker)
        {
          ok = false;
          continue;
        }

        if (!worker->start())
        {
          ok = false;
        }
      }

      if (!ok)
      {
        stop();
      }

      return ok;
    }

    /**
     * @brief Request all workers to stop.
     *
     * This does not forcibly kill threads. Workers exit according to their
     * drain-on-stop configuration.
     */
    void stop() noexcept
    {
      stopping_.store(true, std::memory_order_release);
      running_.store(false, std::memory_order_release);

      for (auto &worker : workers_)
      {
        if (worker)
        {
          worker->stop();
        }
      }
    }

    /**
     * @brief Join all worker threads.
     */
    void join() noexcept
    {
      for (auto &worker : workers_)
      {
        if (worker)
        {
          worker->join();
        }
      }
    }

    /**
     * @brief Submit one task to the scheduler.
     *
     * The scheduler selects a worker and attempts to enqueue the task. If the
     * task cannot be accepted, the configured rejection policy is applied.
     *
     * @param task Task to submit.
     * @return true if the task was accepted or handled by caller_runs/discard.
     */
    [[nodiscard]] bool submit(Task task)
    {
      submitted_tasks_.fetch_add(1, std::memory_order_relaxed);

      if (!task.schedulable())
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return handle_rejected_task(std::move(task));
      }

      if (!running_.load(std::memory_order_acquire) &&
          !task.options().allow_after_stop)
      {
        task.mark_rejected();
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return handle_rejected_task(std::move(task));
      }

      Worker *worker = select_worker(task);
      if (worker == nullptr)
      {
        task.mark_rejected();
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return handle_rejected_task(std::move(task));
      }

      if (!worker->submit(std::move(task)))
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      accepted_tasks_.fetch_add(1, std::memory_order_relaxed);
      return true;
    }

    /**
     * @brief Submit several tasks to the scheduler.
     *
     * @param tasks Tasks to submit.
     * @return Number of tasks accepted or handled successfully.
     */
    [[nodiscard]] std::size_t submit_batch(std::vector<Task> tasks)
    {
      std::size_t accepted = 0;

      for (auto &task : tasks)
      {
        if (submit(std::move(task)))
        {
          ++accepted;
        }
      }

      return accepted;
    }

    /**
     * @brief Remove queued tasks from every worker.
     *
     * @return Number of removed queued tasks.
     */
    [[nodiscard]] std::size_t clear()
    {
      std::size_t removed = 0;

      for (auto &worker : workers_)
      {
        if (worker)
        {
          removed += worker->clear();
        }
      }

      return removed;
    }

    /**
     * @brief Check whether the scheduler is running.
     *
     * @return true if running.
     */
    [[nodiscard]] bool running() const noexcept
    {
      return running_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check whether shutdown was requested.
     *
     * @return true if stopping.
     */
    [[nodiscard]] bool stopping() const noexcept
    {
      return stopping_.load(std::memory_order_acquire);
    }

    /**
     * @brief Return whether all worker queues are empty.
     *
     * @return true if no queued task exists.
     */
    [[nodiscard]] bool empty() const
    {
      for (const auto &worker : workers_)
      {
        if (worker && !worker->empty())
        {
          return false;
        }
      }

      return true;
    }

    /**
     * @brief Return total queued task count across all workers.
     *
     * @return Total pending task count.
     */
    [[nodiscard]] std::size_t size() const
    {
      std::size_t total = 0;

      for (const auto &worker : workers_)
      {
        if (worker)
        {
          total += worker->size();
        }
      }

      return total;
    }

    /**
     * @brief Return the number of workers.
     *
     * @return Worker count.
     */
    [[nodiscard]] std::size_t worker_count() const noexcept
    {
      return workers_.size();
    }

    /**
     * @brief Return the scheduler configuration.
     *
     * @return Scheduler configuration.
     */
    [[nodiscard]] const SchedulerConfig &config() const noexcept
    {
      return config_;
    }

    /**
     * @brief Return a worker by index.
     *
     * @param index Worker index.
     * @return Worker pointer, or nullptr if index is invalid.
     */
    [[nodiscard]] Worker *worker_at(std::size_t index) noexcept
    {
      if (index >= workers_.size())
      {
        return nullptr;
      }

      return workers_[index].get();
    }

    /**
     * @brief Return a worker by index.
     *
     * @param index Worker index.
     * @return Worker pointer, or nullptr if index is invalid.
     */
    [[nodiscard]] const Worker *worker_at(std::size_t index) const noexcept
    {
      if (index >= workers_.size())
      {
        return nullptr;
      }

      return workers_[index].get();
    }

    /**
     * @brief Return aggregate metrics for all workers.
     *
     * @return Threadpool metrics snapshot.
     */
    [[nodiscard]] ThreadPoolMetrics metrics() const
    {
      ThreadPoolMetrics out;
      out.worker_count = workers_.size();
      out.pending_tasks = size();
      out.submitted_tasks = submitted_tasks_.load(std::memory_order_relaxed);
      out.rejected_tasks = rejected_tasks_.load(std::memory_order_relaxed);

      for (const auto &worker : workers_)
      {
        if (!worker)
        {
          continue;
        }

        const WorkerMetrics wm = worker->metrics();

        out.completed_tasks += wm.completed_tasks;
        out.failed_tasks += wm.failed_tasks;
        out.cancelled_tasks += wm.cancelled_tasks;
        out.timed_out_tasks += wm.timed_out_tasks;

        out.active_tasks += wm.active_tasks;

        if (wm.active_tasks > 0)
        {
          ++out.busy_workers;
        }
        else if (wm.state == WorkerState::idle)
        {
          ++out.idle_workers;
        }
      }

      return out;
    }

    /**
     * @brief Return aggregate historical stats for all workers.
     *
     * @return Threadpool stats snapshot.
     */
    [[nodiscard]] ThreadPoolStats stats() const
    {
      ThreadPoolStats out;
      out.accepted_tasks = accepted_tasks_.load(std::memory_order_relaxed);
      out.rejected_tasks = rejected_tasks_.load(std::memory_order_relaxed);

      for (const auto &worker : workers_)
      {
        if (!worker)
        {
          continue;
        }

        const WorkerMetrics wm = worker->metrics();

        out.completed_tasks += wm.completed_tasks;
        out.failed_tasks += wm.failed_tasks;
        out.cancelled_tasks += wm.cancelled_tasks;
        out.timed_out_tasks += wm.timed_out_tasks;
        out.idle_waits += wm.idle_cycles;
      }

      return out;
    }

    /**
     * @brief Return number of submitted task attempts.
     *
     * @return Submitted task count.
     */
    [[nodiscard]] std::uint64_t submitted_tasks() const noexcept
    {
      return submitted_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of accepted tasks.
     *
     * @return Accepted task count.
     */
    [[nodiscard]] std::uint64_t accepted_tasks() const noexcept
    {
      return accepted_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of rejected tasks.
     *
     * @return Rejected task count.
     */
    [[nodiscard]] std::uint64_t rejected_tasks() const noexcept
    {
      return rejected_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of tasks executed on caller thread by rejection policy.
     *
     * @return Caller-runs task count.
     */
    [[nodiscard]] std::uint64_t caller_runs_tasks() const noexcept
    {
      return caller_runs_tasks_.load(std::memory_order_relaxed);
    }

  private:
    /**
     * @brief Select a worker for one task.
     *
     * @param task Task being scheduled.
     * @return Selected worker, or nullptr.
     */
    [[nodiscard]] Worker *select_worker(const Task &task)
    {
      if (workers_.empty())
      {
        return nullptr;
      }

      switch (config_.scheduling_policy)
      {
      case SchedulingPolicy::affinity:
        if (Worker *worker = select_by_affinity(task))
        {
          return worker;
        }
        return select_round_robin();

      case SchedulingPolicy::least_loaded:
        return select_least_loaded();

      case SchedulingPolicy::affinity_then_least_loaded:
        if (Worker *worker = select_by_affinity(task))
        {
          return worker;
        }
        return select_least_loaded();

      case SchedulingPolicy::round_robin:
      default:
        return select_round_robin();
      }
    }

    /**
     * @brief Select a worker using task affinity.
     *
     * @param task Task with optional affinity.
     * @return Selected worker, or nullptr when no valid affinity exists.
     */
    [[nodiscard]] Worker *select_by_affinity(const Task &task) noexcept
    {
      if (!task.options().has_affinity() || workers_.empty())
      {
        return nullptr;
      }

      const std::size_t index =
          static_cast<std::size_t>(task.options().affinity - 1u) %
          workers_.size();

      return workers_[index].get();
    }

    /**
     * @brief Select a worker using round-robin.
     *
     * @return Selected worker, or nullptr.
     */
    [[nodiscard]] Worker *select_round_robin() noexcept
    {
      if (workers_.empty())
      {
        return nullptr;
      }

      const std::size_t index =
          next_worker_.fetch_add(1, std::memory_order_relaxed) %
          workers_.size();

      return workers_[index].get();
    }

    /**
     * @brief Select the worker with the smallest local queue.
     *
     * @return Selected worker, or nullptr.
     */
    [[nodiscard]] Worker *select_least_loaded() noexcept
    {
      if (workers_.empty())
      {
        return nullptr;
      }

      Worker *best = nullptr;
      std::size_t bestSize = 0;

      for (auto &worker : workers_)
      {
        if (!worker)
        {
          continue;
        }

        const std::size_t currentSize = worker->size();

        if (best == nullptr || currentSize < bestSize)
        {
          best = worker.get();
          bestSize = currentSize;
        }
      }

      return best;
    }

    /**
     * @brief Apply the configured rejection policy.
     *
     * @param task Rejected task.
     * @return true if the rejection was handled as a successful submission.
     */
    [[nodiscard]] bool handle_rejected_task(Task task)
    {
      switch (config_.rejection_policy)
      {
      case RejectionPolicy::caller_runs:
        caller_runs_tasks_.fetch_add(1, std::memory_order_relaxed);
        (void)task.run();
        return true;

      case RejectionPolicy::discard:
        return true;

      case RejectionPolicy::reject:
      default:
        return false;
      }
    }

  private:
    /**
     * @brief Normalized scheduler configuration.
     */
    SchedulerConfig config_;

    /**
     * @brief Owned workers.
     */
    std::vector<std::unique_ptr<Worker>> workers_;

    /**
     * @brief Round-robin cursor.
     */
    std::atomic<std::size_t> next_worker_;

    /**
     * @brief Whether the scheduler is accepting ordinary work.
     */
    std::atomic<bool> running_;

    /**
     * @brief Whether stop has been requested.
     */
    std::atomic<bool> stopping_;

    /**
     * @brief Submitted task attempts.
     */
    std::atomic<std::uint64_t> submitted_tasks_;

    /**
     * @brief Accepted task count.
     */
    std::atomic<std::uint64_t> accepted_tasks_;

    /**
     * @brief Rejected task count.
     */
    std::atomic<std::uint64_t> rejected_tasks_;

    /**
     * @brief Number of rejected tasks run on the caller thread.
     */
    std::atomic<std::uint64_t> caller_runs_tasks_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_SCHEDULER_HPP
