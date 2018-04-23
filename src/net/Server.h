#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>
#include <sys/time.h>

#define BUFSIZE 2048

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
namespace net {

	enum State { stopped, starting, running, stopping, error };

	class Server {
	public:
		Server(unsigned int port);
		void start();
		void stop();
		bool send();
		State getState();

	private:
		int sockFD;
    	unsigned int port;
    	State state;
    	struct sockaddr_in myAddr;
    	std::thread* serverThread;
    	bool shouldRun;

    	void setState(State state);
    	void contReadStart();
    	void startTask();
	};	
}