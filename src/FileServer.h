#pragma once
#include "net/State.h"
#include "net/Server.h"
#include <iostream>

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
	
};