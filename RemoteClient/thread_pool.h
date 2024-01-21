#pragma once
#include <functional>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <queue>

class thread_pool {
    using Task = std::function<void()>;

public:
    thread_pool() : thread_pool(max(1, (int)std::thread::hardware_concurrency())) {}

    thread_pool(int n_threads) : n_threads_(n_threads), is_running_(true) {
        init();
    }

    ~thread_pool() {
        is_running_ = false;
        connQueueNotEmpty_.notify_all();
        for (auto& t : vec_thread_)
            if (t.joinable())
                t.join();
    }

    template<typename F, typename ...Args>
    void add_task(F f, Args ...args) {
        std::lock_guard<std::mutex> _lock_queueTask(mtx_queueTask_);
        queue_task_.push(move(bind(f, args...)));
        connQueueNotEmpty_.notify_one();
    }

    template<typename F, typename ...Args>
    bool try_to_add_task(F f, Args ...args) {
        std::unique_lock<std::mutex> lk(mtx_queueTask_, std::try_to_lock);
        if (!lk.owns_lock())
            return false;
        queue_task_.push(move(bind(f, args...)));
        connQueueNotEmpty_.notify_one();
        return true;
    }

    // template<typename F, typename ...Args>
    // auto addTask (F f, Args ...args) {
    //     unique_lock<mutex> _lock_queueTask(mtx_queueTask_);
    //     auto func = packaged_task<function<decltype(F(args...))()>>(f);
    //     queue_task_.push(func);
    //     return func.get_future();
    // }

private:
    int n_threads_;
    std::vector<std::thread> vec_thread_;
    std::queue<Task> queue_task_;
    std::mutex mtx_queueTask_;
    std::atomic<bool> is_running_;
    std::condition_variable connQueueNotEmpty_;

    void init() {
        for (int i = 0; i < n_threads_; i++) {
            std::thread t(&thread_pool::thread_routine, this);
            vec_thread_.emplace_back(std::move(t));
        }
    }

    void thread_routine() {
        Task task;
        while (1) {
            {
                std::unique_lock<std::mutex> _lock_queueTask(mtx_queueTask_);
                connQueueNotEmpty_.wait(_lock_queueTask, [this]() { return !is_running_ || !queue_task_.empty(); });
                if (!is_running_ && queue_task_.empty())
                    break;
                task = move(queue_task_.front());
                queue_task_.pop();
            }
            task();
        }
    }

};
