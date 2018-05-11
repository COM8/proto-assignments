#pragma once
#include <string>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <stdlib.h>
#include <mutex>
#include "Filesystem.h"
#include "net/Client.h"
#include "net/Server.h"
#include "net/ClientHelloMessage.h"
#include "net/ServerHelloMessage.h"
#include "net/PingMessage.h"
#include "net/AckMessage.h"
#include "net/TransferEndedMessage.h"
#include "net/FileCreationMessage.h"
#include "net/FileTransferMessage.h"
#include "Queue.h"
#include "WorkingSet.h"

#define MAX_FILE_CHUNK_SIZE_IN_BYTE 500

enum TransferState
{
	disconnected,
	sendHandshake,
	connected,
	sendingFS,
	awaitingAck,
	ping
};

class FileClient
{
public:
	FileClient(std::string* serverAddress, unsigned short serverPort, FilesystemClient* fS);
	void startSendingFS();
	void stopSendingFS();
	void pingServer();
	void printToDo();
	TransferState getState();

private:
	unsigned short serverPort;
	std::string *serverAddress;
	FilesystemClient *fS;
	WorkingSet *curWorkingSet;
	TransferState state;
	net::Client *client;
	net::Client *uploadClient;
	net::Server *server;
	Queue<net::ReadMessage> *cpQueue;
	SendMessageQueue *sendMessageQueue;
	bool shouldConsumerRun;
	bool shouldTransferRun;
	bool shouldHelloRun;
	std::thread *consumerThread;
	std::thread *helloThread;
	unsigned int seqNumber;
	unsigned int clientId;
	unsigned short listeningPort;
	std::mutex* seqNumberMutex;

	void startConsumerThread();
	void stopConsumerThread();
	void startHelloThread();
	void stopHelloThread();
	unsigned int getNextSeqNumber();
	void helloTask(unsigned short listenPort);
	void consumerTask();
	void onServerHelloMessage(net::ReadMessage *msg);
	void onAckMessage(net::ReadMessage *msg);
	void onTransferEndedMessage(net::ReadMessage *msg);
	void sendNextFilePart();
	void sendClientHelloMessage(unsigned short listeningPort, net::Client *client);
	void sendPingMessage(unsigned int plLength, unsigned int seqNumber, net::Client *client);	
	void sendFolderCreationMessage(struct Folder *f, net::Client *client);
	void sendFileCreationMessage(std::string fid, struct File *f, net::Client *client);
	bool sendNextFilePart(std::string fid, struct File *f, int nextPartNr, net::Client *client);
};