/**
 *
 * @file Worker.hpp
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
#ifndef VIX_THREADPOOL_WORKER_HPP
#define VIX_THREADPOOL_WORKER_HPP

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskQueue.hpp>
#include <vix/threadpool/WorkerId.hpp>
#include <vix/threadpool/WorkerState.hpp>
#include <vix/threadpool/WorkerThread.hpp>
#include <vix/threadpool/detail/WaitStrategy.hpp>
#include <vix/threadpool/this_worker.hpp>

namespace vix::threadpool
{
  /**
   * @brief Worker-local metrics snapshot.
   *
   * WorkerMetrics describes the current and historical state of one worker.
   */
  struct WorkerMetrics
  {
    /**
     * @brief Worker identifier.
     */
    WorkerId id;

    /**
     * @brief Worker index inside the owning pool.
     */
    std::size_t index;

    /**
     * @brief Current worker state.
     */
    WorkerState state;

    /**
     * @brief Number of queued tasks owned by this worker.
     */
    std::size_t pending_tasks;

    /**
     * @brief Total number of tasks accepted by this worker.
     */
    std::uint64_t accepted_tasks;

    /**
     * @brief Total number of tasks executed by this worker.
     */
    std::uint64_t executed_tasks;

    /**
     * @brief Total number of tasks completed successfully.
     */
    std::uint64_t completed_tasks;

    /**
     * @brief Total number of tasks that failed.
     */
    std::uint64_t failed_tasks;

    /**
     * @brief Total number of tasks cancelled before or during execution.
     */
    std::uint64_t cancelled_tasks;

    /**
     * @brief Total number of tasks observed as timed out.
     */
    std::uint64_t timed_out_tasks;

    /**
     * @brief Total number of rejected tasks.
     */
    std::uint64_t rejected_tasks;

    /**
     * @brief Total number of idle loop cycles.
     */
    std::uint64_t idle_cycles;

    /**
     * @brief Construct an empty worker metrics snapshot.
     */
    constexpr WorkerMetrics() noexcept
        : id(invalid_worker_id),
          index(0),
          state(WorkerState::created),
          pending_tasks(0),
          accepted_tasks(0),
          executed_tasks(0),
          completed_tasks(0),
          failed_tasks(0),
          cancelled_tasks(0),
          timed_out_tasks(0),
          rejected_tasks(0),
          idle_cycles(0)
    {
    }
  };

  /**
   * @brief One executable worker owned by a thread pool.
   *
   * Worker owns:
   * - one WorkerThread
   * - one local TaskQueue
   * - worker-local counters
   * - worker lifecycle state
   *
   * The worker accepts tasks through submit(), executes them on its owned
   * thread, and reports completion through an optional finish callback.
   */
  class Worker
  {
  public:
    /**
     * @brief Callback invoked after a task reaches a terminal state.
     */
    using FinishCallback = std::function<void(const Task &)>;

    /**
     * @brief Construct a worker.
     *
     * @param id Worker identifier.
     * @param index Worker index inside the owning pool.
     * @param maxQueueSize Maximum local queue size. Zero means unbounded.
     * @param name Worker thread name.
     */
    Worker(
        WorkerId id,
        std::size_t index,
        std::size_t maxQueueSize = 0,
        std::string name = {})
        : id_(id),
          index_(index),
          queue_(maxQueueSize),
          thread_(id, index, std::move(name)),
          wait_strategy_(),
          finish_callback_(),
          mutex_(),
          cv_(),
          stopping_(false),
          drain_on_stop_(true),
          accepted_tasks_(0),
          executed_tasks_(0),
          completed_tasks_(0),
          failed_tasks_(0),
          cancelled_tasks_(0),
          timed_out_tasks_(0),
          rejected_tasks_(0),
          idle_cycles_(0)
    {
    }

    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;

    Worker(Worker &&) = delete;
    Worker &operator=(Worker &&) = delete;

    /**
     * @brief Stop and join the worker on destruction.
     */
    ~Worker() noexcept
    {
      stop();
      join();
    }

    /**
     * @brief Start the worker thread.
     *
     * Calling start() on an already running worker has no effect.
     *
     * @return true if a new worker thread was started.
     */
    [[nodiscard]] bool start()
    {
      stopping_.store(false, std::memory_order_release);

      return thread_.start(
          [this]()
          {
            run_loop();
          });
    }

    /**
     * @brief Request the worker to stop.
     *
     * This function does not forcibly kill the thread. The worker loop observes
     * the stop request and exits safely.
     */
    void stop() noexcept
    {
      stopping_.store(true, std::memory_order_release);
      thread_.stop();
      cv_.notify_all();
    }

    /**
     * @brief Join the worker thread if joinable.
     */
    void join() noexcept
    {
      thread_.join();
    }

    /**
     * @brief Submit one task to this worker.
     *
     * Tasks are rejected if:
     * - the task is invalid
     * - the worker is stopping and the task does not allow after-stop execution
     * - the queue is full
     *
     * @param task Task to enqueue.
     * @return true if the task was accepted.
     */
    [[nodiscard]] bool submit(Task task)
    {
      if (!task.schedulable())
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      if (stopping_.load(std::memory_order_acquire) &&
          !task.options().allow_after_stop)
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      const bool accepted = queue_.push(std::move(task));
      if (!accepted)
      {
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      accepted_tasks_.fetch_add(1, std::memory_order_relaxed);
      cv_.notify_one();

      return true;
    }

    /**
     * @brief Submit several tasks to this worker.
     *
     * @param tasks Tasks to enqueue.
     * @return Number of accepted tasks.
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
     * @brief Try to pop one task from the worker queue.
     *
     * This function is mainly intended for scheduler or testing internals.
     *
     * @return Task if one is available.
     */
    [[nodiscard]] std::optional<Task> try_pop()
    {
      return queue_.pop();
    }

    /**
     * @brief Clear all queued tasks.
     *
     * @return Number of removed tasks.
     */
    [[nodiscard]] std::size_t clear()
    {
      return queue_.clear();
    }

    /**
     * @brief Set the callback invoked after each task finishes.
     *
     * @param callback Completion callback.
     */
    void set_finish_callback(FinishCallback callback)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      finish_callback_ = std::move(callback);
    }

    /**
     * @brief Set whether queued tasks should be drained when stop is requested.
     *
     * @param value Drain flag.
     */
    void set_drain_on_stop(bool value) noexcept
    {
      drain_on_stop_.store(value, std::memory_order_release);
    }

    /**
     * @brief Check whether the worker drains queued tasks during shutdown.
     *
     * @return true if drain-on-stop is enabled.
     */
    [[nodiscard]] bool drain_on_stop() const noexcept
    {
      return drain_on_stop_.load(std::memory_order_acquire);
    }

    /**
     * @brief Return whether stop was requested.
     *
     * @return true if the worker is stopping.
     */
    [[nodiscard]] bool stopping() const noexcept
    {
      return stopping_.load(std::memory_order_acquire) ||
             thread_.stop_requested();
    }

    /**
     * @brief Check whether the worker thread is joinable.
     *
     * @return true if the underlying thread is joinable.
     */
    [[nodiscard]] bool joinable() const noexcept
    {
      return thread_.joinable();
    }

    /**
     * @brief Return the worker id.
     *
     * @return Worker identifier.
     */
    [[nodiscard]] WorkerId id() const noexcept
    {
      return id_;
    }

    /**
     * @brief Return the worker index.
     *
     * @return Worker index.
     */
    [[nodiscard]] std::size_t index() const noexcept
    {
      return index_;
    }

    /**
     * @brief Return the current worker state.
     *
     * @return Worker state.
     */
    [[nodiscard]] WorkerState state() const noexcept
    {
      return thread_.state();
    }

    /**
     * @brief Return the worker thread name.
     *
     * @return Worker name.
     */
    [[nodiscard]] const std::string &name() const noexcept
    {
      return thread_.name();
    }

    /**
     * @brief Set the worker thread name.
     *
     * @param value New thread name.
     */
    void set_name(std::string value)
    {
      thread_.set_name(std::move(value));
    }

    /**
     * @brief Return whether the local queue is empty.
     *
     * @return true if no task is queued.
     */
    [[nodiscard]] bool empty() const
    {
      return queue_.empty();
    }

    /**
     * @brief Return whether the local queue is full.
     *
     * @return true if the queue reached its configured capacity.
     */
    [[nodiscard]] bool full() const
    {
      return queue_.full();
    }

    /**
     * @brief Return the number of queued tasks.
     *
     * @return Queue size.
     */
    [[nodiscard]] std::size_t size() const
    {
      return queue_.size();
    }

    /**
     * @brief Return the local queue capacity.
     *
     * @return Maximum queue size. Zero means unbounded.
     */
    [[nodiscard]] std::size_t max_queue_size() const noexcept
    {
      return queue_.max_size();
    }

    /**
     * @brief Update the local queue capacity.
     *
     * @param value New maximum queue size. Zero means unbounded.
     */
    void set_max_queue_size(std::size_t value) noexcept
    {
      queue_.set_max_size(value);
    }

    /**
     * @brief Return worker-local metrics.
     *
     * @return Worker metrics snapshot.
     */
    [[nodiscard]] WorkerMetrics metrics() const
    {
      WorkerMetrics out;
      out.id = id_;
      out.index = index_;
      out.state = state();
      out.pending_tasks = queue_.size();
      out.accepted_tasks = accepted_tasks_.load(std::memory_order_relaxed);
      out.executed_tasks = executed_tasks_.load(std::memory_order_relaxed);
      out.completed_tasks = completed_tasks_.load(std::memory_order_relaxed);
      out.failed_tasks = failed_tasks_.load(std::memory_order_relaxed);
      out.cancelled_tasks = cancelled_tasks_.load(std::memory_order_relaxed);
      out.timed_out_tasks = timed_out_tasks_.load(std::memory_order_relaxed);
      out.rejected_tasks = rejected_tasks_.load(std::memory_order_relaxed);
      out.idle_cycles = idle_cycles_.load(std::memory_order_relaxed);
      return out;
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
     * @brief Return number of executed tasks.
     *
     * @return Executed task count.
     */
    [[nodiscard]] std::uint64_t executed_tasks() const noexcept
    {
      return executed_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of completed tasks.
     *
     * @return Completed task count.
     */
    [[nodiscard]] std::uint64_t completed_tasks() const noexcept
    {
      return completed_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of failed tasks.
     *
     * @return Failed task count.
     */
    [[nodiscard]] std::uint64_t failed_tasks() const noexcept
    {
      return failed_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of cancelled tasks.
     *
     * @return Cancelled task count.
     */
    [[nodiscard]] std::uint64_t cancelled_tasks() const noexcept
    {
      return cancelled_tasks_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return number of timed out tasks.
     *
     * @return Timed out task count.
     */
    [[nodiscard]] std::uint64_t timed_out_tasks() const noexcept
    {
      return timed_out_tasks_.load(std::memory_order_relaxed);
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
     * @brief Return number of idle cycles.
     *
     * @return Idle cycle count.
     */
    [[nodiscard]] std::uint64_t idle_cycles() const noexcept
    {
      return idle_cycles_.load(std::memory_order_relaxed);
    }

  private:
    /**
     * @brief Main worker loop.
     *
     * The loop waits for tasks, executes them, updates metrics, and exits when
     * stop is requested. If drain-on-stop is enabled, queued tasks are completed
     * before the loop exits.
     */
    void run_loop()
    {
      std::uint32_t idleStreak = 0;

      while (should_continue_loop())
      {
        std::optional<Task> task = queue_.pop();

        if (!task.has_value())
        {
          thread_.set_state(WorkerState::idle);
          ++idleStreak;
          idle_cycles_.fetch_add(1, std::memory_order_relaxed);
          wait_for_work(idleStreak);
          continue;
        }

        idleStreak = 0;
        execute_task(*task);
      }
    }

    /**
     * @brief Check whether the worker loop should continue.
     *
     * @return true if the loop should keep running.
     */
    [[nodiscard]] bool should_continue_loop() const
    {
      if (!stopping())
      {
        return true;
      }

      return drain_on_stop_.load(std::memory_order_acquire) && !queue_.empty();
    }

    /**
     * @brief Wait for work or stop notification.
     *
     * @param idleStreak Consecutive idle loop count.
     */
    void wait_for_work(std::uint32_t idleStreak)
    {
      {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!queue_.empty() || stopping())
        {
          return;
        }

        cv_.wait_for(
            lock,
            wait_strategy_.short_sleep(),
            [this]()
            {
              return !queue_.empty() || stopping();
            });
      }

      if (queue_.empty())
      {
        wait_strategy_.wait(idleStreak);
      }
    }

    /**
     * @brief Execute one task and update worker metrics.
     *
     * @param task Task to execute.
     */
    void execute_task(Task &task)
    {
      thread_.set_state(WorkerState::running);
      this_worker::set_task(task.id());

      executed_tasks_.fetch_add(1, std::memory_order_relaxed);

      const TaskResult result = task.run();

      switch (result)
      {
      case TaskResult::success:
        completed_tasks_.fetch_add(1, std::memory_order_relaxed);
        break;

      case TaskResult::failure:
        failed_tasks_.fetch_add(1, std::memory_order_relaxed);
        break;

      case TaskResult::cancelled:
        cancelled_tasks_.fetch_add(1, std::memory_order_relaxed);
        break;

      case TaskResult::timeout:
        timed_out_tasks_.fetch_add(1, std::memory_order_relaxed);
        break;

      case TaskResult::rejected:
        rejected_tasks_.fetch_add(1, std::memory_order_relaxed);
        break;

      case TaskResult::none:
      default:
        failed_tasks_.fetch_add(1, std::memory_order_relaxed);
        break;
      }

      notify_finished(task);

      this_worker::clear_task();

      if (!stopping())
      {
        thread_.set_state(WorkerState::idle);
      }
    }

    /**
     * @brief Invoke the finish callback for one completed task.
     *
     * @param task Completed task.
     */
    void notify_finished(const Task &task)
    {
      FinishCallback callback;

      {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = finish_callback_;
      }

      if (!callback)
      {
        return;
      }

      try
      {
        callback(task);
      }
      catch (...)
      {
      }
    }

  private:
    /**
     * @brief Worker identifier.
     */
    WorkerId id_;

    /**
     * @brief Worker index inside the owning pool.
     */
    std::size_t index_;

    /**
     * @brief Local task queue.
     */
    TaskQueue queue_;

    /**
     * @brief Owned worker thread.
     */
    WorkerThread thread_;

    /**
     * @brief Idle wait strategy.
     */
    detail::WaitStrategy wait_strategy_;

    /**
     * @brief Optional task completion callback.
     */
    FinishCallback finish_callback_;

    /**
     * @brief Mutex used for waiting and callback protection.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable used to wake the worker.
     */
    std::condition_variable cv_;

    /**
     * @brief Whether stop has been requested.
     */
    std::atomic<bool> stopping_;

    /**
     * @brief Whether queued work should be drained during shutdown.
     */
    std::atomic<bool> drain_on_stop_;

    /**
     * @brief Accepted task counter.
     */
    std::atomic<std::uint64_t> accepted_tasks_;

    /**
     * @brief Executed task counter.
     */
    std::atomic<std::uint64_t> executed_tasks_;

    /**
     * @brief Completed task counter.
     */
    std::atomic<std::uint64_t> completed_tasks_;

    /**
     * @brief Failed task counter.
     */
    std::atomic<std::uint64_t> failed_tasks_;

    /**
     * @brief Cancelled task counter.
     */
    std::atomic<std::uint64_t> cancelled_tasks_;

    /**
     * @brief Timed out task counter.
     */
    std::atomic<std::uint64_t> timed_out_tasks_;

    /**
     * @brief Rejected task counter.
     */
    std::atomic<std::uint64_t> rejected_tasks_;

    /**
     * @brief Idle cycle counter.
     */
    std::atomic<std::uint64_t> idle_cycles_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_WORKER_HPP
