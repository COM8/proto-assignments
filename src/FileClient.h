#pragma once
#include <string>
#include <unistd.h>
#include <iostream>
#include "Filesystem.h"
#include "net/Client.h"
#include "net/Server.h"
#include "net/ClientHelloMessage.h"
#include "net/ServerHelloMessage.h"
#include "net/PingMessage.h"
#include "Queue.h"

enum TransferState
{
	disconnected,
	sendHandshake,
	connected
};

class FileClient
{
public:
	FileClient(std::string* serverAddress, unsigned short serverPort, Filesystem* fs);
	void startSendingFS();
	void stopSendingFS();

private:
	unsigned short serverPort;
	std::string* serverAddress;
	Filesystem* fs;
	TransferState state;
	net::Client client;
	net::Server server;
	Queue<net::ReadMessage>* cpQueue;
	bool shouldConsumerRun;
	bool shouldTransferRun;
	std::thread* consumerThread;
	unsigned int seqNumber;

	void sendClientHelloMessage(unsigned short listeningPort);
	void startConsumerThread();
	void stopConsumerThread();
	void consumerTask();
	void onServerHelloMessage(net::ReadMessage& msg);
	void transferFiles();
	void sendPingMessage(unsigned int plLength, unsigned int seqNumber);
};