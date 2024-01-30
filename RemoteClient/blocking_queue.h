#pragma once
#include <queue>
#include <mutex>
#include <optional>
#include <condition_variable>

/*
* 线程安全的阻塞队列
*/
template<typename T>
class blocking_queue {
private:
	std::queue<T> queue_;
	std::mutex mtx_;
	std::condition_variable connVar_;
	size_t max_size_;
	std::atomic<bool> thread_io_terminated_ = false;

public:
	blocking_queue(size_t max_size) : max_size_(max_size) {}


	size_t size() {
		std::unique_lock<std::mutex> lck(mtx_);
		return queue_.size();
	}
	
	bool enqueue(T item) {
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (thread_io_terminated_)
				return false;
			while (max_size_ && queue_.size() >= max_size_)
				queue_.pop();
			queue_.push(std::move(item));
		}
		connVar_.notify_one();

		return true;
	}

	std::optional<T> dequeue() {
		std::unique_lock<std::mutex> lock(mtx_);
		connVar_.wait(lock, [this] { return !queue_.empty() || thread_io_terminated_; });
		if (thread_io_terminated_) 
			return std::nullopt;

		auto item = std::move(queue_.front());
		queue_.pop();
		return item;
	}

	void reset() {
		{
			std::lock_guard<std::mutex> lcok(mtx_);
			while (!queue_.empty())
				queue_.pop();
			thread_io_terminated_ = false;
		}
		connVar_.notify_all();
	}

	void wait_until_running() {
		std::unique_lock<std::mutex> lock(mtx_);
		if (!thread_io_terminated_.load())
			return;
		connVar_.wait(lock, [this] { return !thread_io_terminated_.load(); });
	}

	void terminate() {
		{
			std::lock_guard<std::mutex> lock(mtx_);
			thread_io_terminated_ = true;
		}
		connVar_.notify_all();
	}

	bool isTerminated() const { return thread_io_terminated_.load(); }
};

