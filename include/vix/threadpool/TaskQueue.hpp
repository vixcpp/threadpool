/**
 *
 * @file TaskQueue.hpp
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
#ifndef VIX_THREADPOOL_TASK_QUEUE_HPP
#define VIX_THREADPOOL_TASK_QUEUE_HPP

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <vix/threadpool/Task.hpp>
#include <vix/threadpool/TaskCmp.hpp>

namespace vix::threadpool
{
  /**
   * @brief Thread-safe priority task queue.
   *
   * TaskQueue stores move-only Task objects and orders them using TaskCmp.
   *
   * It intentionally uses a vector heap instead of std::priority_queue because
   * std::priority_queue::top() returns a const reference, which makes extracting
   * move-only tasks awkward. With a manual heap, pop() can move the selected task
   * out safely.
   */
  class TaskQueue
  {
  public:
    /**
     * @brief Construct an unbounded task queue.
     */
    TaskQueue() noexcept
        : tasks_(),
          max_size_(0),
          cmp_()
    {
    }

    /**
     * @brief Construct a task queue with an optional capacity.
     *
     * @param maxSize Maximum number of queued tasks. Zero means unbounded.
     */
    explicit TaskQueue(std::size_t maxSize) noexcept
        : tasks_(),
          max_size_(maxSize),
          cmp_()
    {
    }

    TaskQueue(const TaskQueue &) = delete;
    TaskQueue &operator=(const TaskQueue &) = delete;

    TaskQueue(TaskQueue &&) = delete;
    TaskQueue &operator=(TaskQueue &&) = delete;

    /**
     * @brief Destroy the task queue.
     */
    ~TaskQueue() = default;

    /**
     * @brief Push a task into the queue.
     *
     * Invalid, terminal, or over-capacity tasks are rejected.
     * Accepted tasks are marked as queued.
     *
     * @param task Task to enqueue.
     * @return true if the task was accepted, false otherwise.
     */
    [[nodiscard]] bool push(Task task)
    {
      if (!task.schedulable())
      {
        return false;
      }

      std::lock_guard<std::mutex> lock(mutex_);

      if (full_unlocked())
      {
        return false;
      }

      task.mark_queued();
      tasks_.push_back(std::move(task));
      std::push_heap(tasks_.begin(), tasks_.end(), cmp_);

      return true;
    }

    /**
     * @brief Push several tasks into the queue.
     *
     * @param tasks Tasks to enqueue.
     * @return Number of accepted tasks.
     */
    [[nodiscard]] std::size_t push_batch(std::vector<Task> tasks)
    {
      std::size_t accepted = 0;

      std::lock_guard<std::mutex> lock(mutex_);

      for (auto &task : tasks)
      {
        if (!task.schedulable())
        {
          continue;
        }

        if (full_unlocked())
        {
          break;
        }

        task.mark_queued();
        tasks_.push_back(std::move(task));
        ++accepted;
      }

      if (accepted > 0)
      {
        std::make_heap(tasks_.begin(), tasks_.end(), cmp_);
      }

      return accepted;
    }

    /**
     * @brief Pop the highest-priority task.
     *
     * @return Next task if one exists, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<Task> pop()
    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (tasks_.empty())
      {
        return std::nullopt;
      }

      std::pop_heap(tasks_.begin(), tasks_.end(), cmp_);

      Task task = std::move(tasks_.back());
      tasks_.pop_back();

      return task;
    }

    /**
     * @brief Inspect the next task without removing it.
     *
     * @return Pointer to the next task, or nullptr if the queue is empty.
     *
     * @warning The returned pointer becomes invalid after any queue mutation.
     */
    [[nodiscard]] const Task *peek() const
    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (tasks_.empty())
      {
        return nullptr;
      }

      return &tasks_.front();
    }

    /**
     * @brief Remove all queued tasks.
     *
     * @return Number of removed tasks.
     */
    [[nodiscard]] std::size_t clear()
    {
      std::lock_guard<std::mutex> lock(mutex_);

      const std::size_t count = tasks_.size();
      tasks_.clear();

      return count;
    }

    /**
     * @brief Check whether the queue is empty.
     *
     * @return true if no task is queued.
     */
    [[nodiscard]] bool empty() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return tasks_.empty();
    }

    /**
     * @brief Check whether the queue reached its configured capacity.
     *
     * @return true if bounded and full.
     */
    [[nodiscard]] bool full() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return full_unlocked();
    }

    /**
     * @brief Return the current queue size.
     *
     * @return Number of queued tasks.
     */
    [[nodiscard]] std::size_t size() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return tasks_.size();
    }

    /**
     * @brief Return the queue capacity.
     *
     * @return Maximum queue size. Zero means unbounded.
     */
    [[nodiscard]] std::size_t max_size() const noexcept
    {
      return max_size_;
    }

    /**
     * @brief Check whether the queue has a fixed capacity.
     *
     * @return true if max_size() is greater than zero.
     */
    [[nodiscard]] bool bounded() const noexcept
    {
      return max_size_ > 0;
    }

    /**
     * @brief Update the queue capacity.
     *
     * A value of zero makes the queue unbounded. If the new capacity is lower
     * than the current queue size, existing tasks are kept, but new pushes are
     * rejected until the size drops below the capacity.
     *
     * @param value New maximum size.
     */
    void set_max_size(std::size_t value) noexcept
    {
      max_size_ = value;
    }

  private:
    /**
     * @brief Check whether the queue is full.
     *
     * Caller must already hold the mutex.
     *
     * @return true if bounded and full.
     */
    [[nodiscard]] bool full_unlocked() const noexcept
    {
      return max_size_ > 0 && tasks_.size() >= max_size_;
    }

  private:
    /**
     * @brief Heap storage for queued tasks.
     */
    std::vector<Task> tasks_;

    /**
     * @brief Maximum queue size. Zero means unbounded.
     */
    std::size_t max_size_;

    /**
     * @brief Comparator used by the heap.
     */
    TaskCmp cmp_;

    /**
     * @brief Mutex protecting queue access.
     */
    mutable std::mutex mutex_;
  };

} // namespace vix::threadpool

#endif // VIX_THREADPOOL_TASK_QUEUE_HPP
