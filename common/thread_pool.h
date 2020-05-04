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

template<typename T = void>
class ThreadPool
{
public:
    typedef std::function<T()> Task;

protected:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::condition_variable consumer_cv_;
    std::queue<Task> task_queue_;
    std::vector<std::thread> threads_;
    int busy_threads_ = 0;
    bool stop_ = false;

    ThreadPool() {}

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
    ThreadPool(std::size_t num_threads)
    {
        try
        {
            for(std::size_t i = 0; i < num_threads; ++i)
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

    ~ThreadPool()
    {
        cancel_tasks();
        stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

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

    void queue_task(const Task& task)
    {
        std::lock_guard lock(mtx_);
        if(stop_)
            throw std::logic_error("Posting to a stopped thread pool.");
        task_queue_.push(task);
        cv_.notify_one();
    }

    void queue_task(Task&& task)
    {
        std::lock_guard lock(mtx_);
        if(stop_)
            throw std::logic_error("Posting to a stopped thread pool.");
        task_queue_.push(std::move(task));
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

template<typename T>
class PackagedThreadPool : public ThreadPool<T>
{
public:
    using ThreadPool::Task;

protected:
    std::queue<std::future<T>> futures_;

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

            std::packaged_task<T(void)> task(std::move(task_queue_.front()));
            task_queue_.pop();

            futures_.push(task.get_future());

            ++busy_threads_;
            lock.unlock();

            task();

            lock.lock();
            --busy_threads_;
            consumer_cv_.notify_all();
        }
    }

public:
    PackagedThreadPool(std::size_t num_threads)
    {
        try
        {
            for(std::size_t i = 0; i < num_threads; ++i)
            {
                threads_.push_back(std::thread(&PackagedThreadPool::worker, this));
            }
        }
        catch(...)
        {
            stop();
            throw;
        }
    }

    bool pop_future(std::future<T>& out)
    {
        std::unique_lock lock(mtx_);

        while(futures_.empty() && !task_queue_.empty())
            consumer_cv_.wait(lock);

        if(futures_.empty())
            return false;

        out = std::move(futures_.front());
        futures_.pop();
        return true;
    }

    std::vector<std::future<T>> get_futures()
    {
        std::unique_lock lock(mtx_);
        std::vector<std::future<T>> vec;

        while(futures_.empty() && !task_queue_.empty())
            consumer_cv_.wait(lock);

        while(!futures_.empty())
        {
            vec.push_back(std::move(futures_.front()));
            futures_.pop();
        }

        return vec;
    }
};
