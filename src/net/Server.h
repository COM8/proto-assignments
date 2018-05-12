#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <sys/time.h>
#include <bitset>
#include "net/State.h"
#include "Helpers.h"
#include "Queue.h"
#include "net/AbstractMessage.h"

#define BUF_SIZE 2048
#define MAX_CLIENT_COUNT = 1;

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
namespace net
{

class Server
{
  public:
	Server(unsigned short port, Queue<net::ReadMessage> *cpQueue);
	Server() = default;
	void start();
	void stop();
	net::State getState();

  private:
	int sockFD;
	unsigned short port;
	net::State state;
	struct sockaddr_in myAddr;
	std::thread *serverThread;
	bool shouldRun;
	Queue<net::ReadMessage> *cpQueue;

	void setState(net::State state);
	void contReadStart();
	void startTask();
	void readNextMessage();
};
}