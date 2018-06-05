#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include "net/AbstractMessage.h"
#include "Helpers.h"
#include "sec/DiffieHellman.h"
#include "Logger.h"

#define BUF_SIZE 2048
#define MESSAGE_DROP_CHANCE 0 // Between 0 and 100 of 100

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
namespace net
{
class Client
{
  public:
	Client(std::string hostAddr, unsigned short port, sec::DiffieHellman *enc);
	Client() = default;
	bool send(net::AbstractMessage *msg);
	bool send(net::Message *msg);

  private:
	int sockFD;
	unsigned short port;
	std::string hostAddr;
	struct sockaddr_in serverAddressStruct;
	sec::DiffieHellman *enc;

	void init();
};
}