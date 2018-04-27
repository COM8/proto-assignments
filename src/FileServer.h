#pragma once

#include <iostream>
#include <thread>
#include "net/State.h"
#include "net/Server.h"
#include "Queue.h"

class FileServer
{
public:
	FileServer(unsigned short port);
	void start();
	void stop();

private:
	unsigned short port;
	net::Server server;
	net::State state;
	Queue<net::ReadMessage>* cpQueue;
	bool shouldConsumerRun;
	std::thread* consumerThread;

	void startConsumerThread();
	void stopConsumerThread();
	void consumerTask();
};