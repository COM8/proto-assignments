#pragma once
#include <string>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <stdlib.h>
#include <mutex>
#include <list>
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
#include "net/FileStatusMessage.h"
#include "Queue.h"
#include "WorkingSet.h"
#include "Timer.h"
#include "TimerTickable.h"

#define MAX_ACK_TIME_IN_S 1
#define ACK_TIMER_IDENT 0
#define PING_TIMER_IDENT 1

enum TransferState
{
	disconnected,
	sendHandshake,
	connected,
	sendFileStatus,
	sendingFS,
	awaitingAck,
	ping
};

class FileClient : public TimerTickable
{
public:
	FileClient(std::string* serverAddress, unsigned short serverPort, FilesystemClient* fS);
	void startSendingFS();
	void stopSendingFS();
	void restartSendingFS();
	void pingServer();
	void printToDo();
	void onTimerTick(int identifier);
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
	bool shouldFileStatusRun;
	std::thread *consumerThread;
	std::thread *helloThread;
	std::thread *fileStatusThread;
	unsigned int seqNumber;
	unsigned int clientId;
	unsigned short listeningPort;
	std::mutex* seqNumberMutex;
	Timer *sendMessageTimer;
	Timer *pingTimer;
	bool transferFinished;
	bool reconnecting;

	void startConsumerThread();
	void stopConsumerThread();
	void startHelloThread();
	void stopHelloThread();
	void setState(TransferState state);
	unsigned int getNextSeqNumber();
	void helloTask(unsigned short listenPort);
	void consumerTask();
	void fileStatusTask();
	void startFileStatusThread();
	void stopFileStatusThread();
	void onServerHelloMessage(net::ReadMessage *msg);
	void onAckMessage(net::ReadMessage *msg);
	void onTransferEndedMessage(net::ReadMessage *msg);
	void onFileStatusMessage(net::ReadMessage *msg);
	void sendNextFilePart();
	void sendClientHelloMessage(unsigned short listeningPort, net::Client *client, unsigned char flags);
	void sendPingMessage(unsigned int plLength, unsigned int seqNumber, net::Client *client);	
	void sendFolderCreationMessage(struct Folder *f, net::Client *client);
	void sendFileCreationMessage(std::string fid, struct File *f, net::Client *client);
	bool sendNextFilePart(std::string fid, struct File *f, unsigned int nextPartNr, net::Client *client);
	void sendTransferEndedMessage(unsigned char flags, net::Client *client);
	void sendFileStatusMessage(unsigned char flags, net::Client *client);
};