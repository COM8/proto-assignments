#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>
#include <sys/time.h>
#include "net/AbstractMessage.h"
#include "net/State.h"

#define BUF_SIZE 2048

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
namespace net {
	class Client {
	public:
		Client(std::string hostAddr, unsigned int port);
		void start();
		void stop();
		bool send(net::AbstractMessage* msg);
		net::State getState();

	private:
		int sockFD;
    	unsigned int port;
    	std::string hostAddr;
    	net::State state;
    	std::thread* clientThread;
    	bool shouldRun;

    	void setState(net::State state);
    	void contReadStart();
    	void startTask();
	};	
}