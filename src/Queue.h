#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// Based on: https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
template <class T> class Queue
{
public:
	Queue();
	~Queue();

	T pop() {
		std::unique_lock<std::mutex> mlock(mutex);
	    while (queue.empty())
	    {
	      condVar.wait(mlock);
	    }
	    auto item = queue.front();
	    queue.pop();
	    return item;
	}

	void pop(T& item) {
		std::unique_lock<std::mutex> mlock(mutex);
	    while (queue.empty())
	    {
	      condVar.wait(mlock);
	    }
	    item = queue.front();
	    queue.pop();
	}
	
	void push(T& item) {
		std::unique_lock<std::mutex> mlock(mutex);
	    queue.push(item);
	    mlock.unlock();
	    condVar.notify_one();
	}

	void clear() {}

private:
	std::queue<T> queue;
	std::mutex mutex;
	std::condition_variable condVar;
	
};