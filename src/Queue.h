#pragma once

#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <time.h>
#include "net/AbstractMessage.h"

// Based on: https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
template <typename T> class Queue {
public:
	Queue() {
		queueMutex = new std::mutex();
		condVar = new std::condition_variable();
	}

	~Queue() {}

	T pop() {
		std::unique_lock<std::mutex> mlock(*queueMutex);
	    while (queue.empty())
	    {
	      condVar->wait(mlock);
	    }
	    auto item = queue.front();
	    queue.pop_front();
	    return item;
	}

	void pop(T& item) {
		std::unique_lock<std::mutex> mlock(*queueMutex);
	    while (queue.empty())
	    {
	      condVar->wait(mlock);
	    }
	    item = queue.front();
	    queue.pop_front();
	}

	void push(T&& item)
  	{
    	std::unique_lock<std::mutex> mlock(*queueMutex);
    	queue.push_back(std::move(item));
    	mlock.unlock();
    	condVar->notify_one();
  	}
	
	void push(T& item) {
		std::unique_lock<std::mutex> mlock(*queueMutex);
	    queue.push_back(item);
	    mlock.unlock();
	    condVar->notify_one();
	}

	void clear() {}

protected:
	std::list<T> queue;
	std::mutex* queueMutex;
	std::condition_variable* condVar;
	
};

struct SendMessage {
	net::AbstractMessage msg;
	unsigned int sequenceNumber;
	time_t sendTime;
};

class SendMessageQueue : Queue<struct SendMessage> {

public:
	SendMessageQueue() {}
	~SendMessageQueue() {}

	void pushSendMessage(int sequenceNumber, AbstractMessage msg) {
		struct SendMessage sM;
		sM.sendTime = time(NULL);
		sM.sequenceNumber = sequenceNumber;
		sM.msg = msg;
		push(sM);
	}

	bool onSequenceNumberAck(unsigned int sequenceNumber) {
		std::unique_lock<std::mutex> mlock(*queueMutex);
		std::list<struct SendMessage>::iterator i = queue.begin();
		bool found = false;
		while (i != queue.end())
		{
			if(i->sequenceNumber == sequenceNumber) {
				queue.erase(i);
				found = true;
				break;
			}
			i++;
		}
    	mlock.unlock();
    	condVar->notify_one();
		return found;
	}

	std::list<struct SendMessage> popNotAckedMessages(uint maxMessageAgeInSec, std::list<struct SendMessage> *tooOldMessages) {
		std::unique_lock<std::mutex> mlock(*queueMutex);
		time_t now = time(NULL);
		std::list<struct SendMessage>::iterator i = queue.begin();
		bool found = false;
		while (i != queue.end())
		{
			if(difftime(now, i->sendTime) > maxMessageAgeInSec) {
				tooOldMessages->push_back(*i);
				queue.erase(i);
				break;
			}
			i++;
		}
    	mlock.unlock();
    	condVar->notify_one();
	}

};