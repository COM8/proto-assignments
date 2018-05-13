#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <cstdint>
#include <time.h>
#include <mutex>
#include "net/State.h"
#include "net/Server.h"
#include "net/Client.h"
#include "Queue.h"
#include "net/ServerHelloMessage.h"
#include "net/FileCreationMessage.h"
#include "net/FileTransferMessage.h"
#include "net/ClientHelloMessage.h"
#include "net/TransferEndedMessage.h"
#include "net/FileStatusMessage.h"
#include "net/PingMessage.h"
#include "net/AckMessage.h"
#include "Filesystem.h"
#include "Timer.h"
#include "TimerTickable.h"

enum FileClientConnectionState {
	c_disconnected,
	c_clientHello,
	c_serverHello,
	c_connected,
	c_ping,
	c_error
};

struct FileClientConnection {
	unsigned int clientId;
	unsigned short portLocal;
	unsigned short portRemote;
	char* remoteIp;
	net::Client* udpClient;
	net::Server* udpServer;
	Queue<net::ReadMessage>* cpQueue;
	FileClientConnectionState state = c_disconnected;
	FilesystemServer *fS;
	time_t lastMessageTime;
	std::string curFID;
	unsigned int lastFIDPartNumber;
	std::uint64_t curFIDLength;
	bool isCurFIDFolder;
};

class FileServer : public TimerTickable
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
	std::unordered_map <unsigned int, FileClientConnection*> *clients;
	std::mutex *clientsMutex;
	Timer *cleanupTimer;

	void startConsumerThread();
	void stopConsumerThread();
	void consumerTask();
	void cleanupClients();
	void disconnectClient(FileClientConnection *client);
	void onTimerTick(int identifier);
	void sendServerHelloMessage(FileClientConnection *client, unsigned char flags);
	void onClientHelloMessage(net::ReadMessage *msg);
	void onPingMessage(net::ReadMessage *msg);
	void onAckMessage(net::ReadMessage *msg);
	void onFileCreationMessage(net::ReadMessage *msg);
	void onFileTransferMessage(net::ReadMessage *msg);
	void onTransferEndedMessage(net::ReadMessage *msg);
	void onFileStatusMessage(net::ReadMessage *msg);
};