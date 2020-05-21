/*
    Copyright 2020 php42

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <future>
#include <utility>
#include <stdexcept>
#include <type_traits>

template<typename FuncType = std::function<void()>>
class ThreadPool
{
public:
    typedef FuncType Task;

protected:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::condition_variable consumer_cv_;
    std::queue<Task> task_queue_;
    std::vector<std::thread> threads_;
    int busy_threads_ = 0;
    bool stop_ = false;

    void worker()
    {
        std::unique_lock lock(mtx_);

        for(;;)
        {
            while(task_queue_.empty())
            {
                if(stop_)
                    return;
                cv_.wait(lock);
            }

            Task task(std::move(task_queue_.front()));
            task_queue_.pop();

            ++busy_threads_;
            lock.unlock();

            task();

            lock.lock();
            --busy_threads_;
            consumer_cv_.notify_all();
        }
    }

public:
    ThreadPool() = default;
    ThreadPool(std::size_t num_threads) { start(num_threads); }

    ~ThreadPool()
    {
        cancel_tasks();
        stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void start(std::size_t num_threads)
    {
        try
        {
            std::lock_guard lock(mtx_);
            stop_ = false;
            while(threads_.size() < num_threads)
            {
                threads_.push_back(std::thread(&ThreadPool::worker, this));
            }
        }
        catch(...)
        {
            stop();
            throw;
        }
    }

    void stop()
    {
        {
            std::lock_guard lock(mtx_);
            stop_ = true;
            cv_.notify_all();
        }

        for(auto& thread : threads_)
        {
            if(thread.joinable())
                thread.join();
        }

        threads_.clear();
        task_queue_ = {};
    }

    template<typename T>
    void queue_task(T&& task)
    {
        std::lock_guard lock(mtx_);
        if(stop_)
            throw std::logic_error("Posting to a stopped thread pool.");
        task_queue_.emplace(std::forward<T>(task));
        cv_.notify_one();
    }

    void cancel_tasks()
    {
        std::lock_guard lock(mtx_);
        task_queue_ = {};
    }

    void wait()
    {
        std::unique_lock lock(mtx_);
        if(stop_)
            throw std::logic_error("Waiting on a stopped thread pool.");
        while(!task_queue_.empty() || busy_threads_)
            consumer_cv_.wait(lock);
    }

    void wait_one()
    {
        std::unique_lock lock(mtx_);
        if(stop_)
            throw std::logic_error("Waiting on a stopped thread pool.");
        if(!task_queue_.empty() || busy_threads_)
            consumer_cv_.wait(lock);
    }
};

template<typename RetType>
class PackagedThreadPool : public ThreadPool<std::packaged_task<RetType()>>
{
public:
    using ThreadPool::Task;

public:
    PackagedThreadPool() = default;
    PackagedThreadPool(std::size_t num_threads) { start(num_threads); }

    template<typename T>
    std::future<RetType> queue_task(T&& task)
    {
        Task t(std::forward<T>(task));
        auto ret = t.get_future();

        std::lock_guard lock(mtx_);
        if(stop_)
            throw std::logic_error("Posting to a stopped thread pool.");
        task_queue_.emplace(std::move(t));
        cv_.notify_one();
        return std::move(ret);
    }
};
