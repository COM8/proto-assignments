#pragma once
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>
#include <sys/time.h>
#include "net/State.h"

#define BUF_SIZE 2048
#define MAX_CLIENT_COUNT = 1;

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
namespace net {

	class Server {
	public:
		Server(unsigned short port);
		Server() = default;
		void start();
		void stop();
		net::State getState();

	private:
		int sockFD;
    	unsigned short port;
    	net::State state;
    	struct sockaddr_in myAddr;
    	std::thread* serverThread;
    	bool shouldRun;

    	void setState(net::State state);
    	void contReadStart();
    	void startTask();
	};	
}