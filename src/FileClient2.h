#pragma once

#include <iostream>
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
#include "Logger.h"

#define MAX_ACK_TIME_IN_S 1
#define TIMER_TICK_INTERVALL_MS 1000
#define MAX_MESSAGE_SEND_TRIES 15

enum FileClient2State
{
    disconnected = 0,
    sendClientHello = 1,
    handshake = 2,
    connected = 3,
    ping = 4,
    reqestedFileStatus = 5,
    sendingFS = 6,
    awaitingAck = 7,
    resending = 8
};

class FileClient2 : public TimerTickable
{
  public:
    FileClient2(std::string serverAddress, unsigned short serverPort, unsigned int clientId, std::string clientPassword, FilesystemClient *fS);
    ~FileClient2();

    void connect();
    void disconnect();
    void onTimerTick(int identifier);
    FileClient2State getState();
    void printToDo();

  private:
    FileClient2State state;
    std::mutex *stateMutex;
    FilesystemClient *fS;
    unsigned short serverPort;
    unsigned short listenPort;
    unsigned short uploadPort;
    std::string serverAddress;
    std::mutex *seqNumberMutex;
    unsigned int seqNumber;
    std::thread *helloThread;
    bool shouldHelloRun;
    std::thread *consumerThread;
    bool shouldConsumerRun;
    unsigned int clientId;
    std::string clientPassword;
    net::Client *client;
    net::Client *uploadClient;
    net::Server *server;
    Queue<net::ReadMessage> *cpQueue;
    SendMessageQueue *sendMessageQueue;
    bool reconnect;
    unsigned int msgTimeoutCount;
	Timer *tTimer;
    WorkingSet *curWorkingSet;
    bool transferFinished;

    void setState(FileClient2State state);
    void startSendingFS();
    unsigned int getNextSeqNumber();
    void startHelloThread();
    void stopHelloThread();
    void helloTask(unsigned short listenPort, bool reconnecting, net::Client *client);
    void stopConsumerThread();
    void startConsumerThread();
    void consumerTask();
    void sendNextFilePart();
    void sendFolderDeletionMessage(std::string folder, net::Client *client);
    void sendFileDeletionMessage(std::string file, net::Client *client);
    void sendTransferEndedMessage(unsigned char flags, net::Client *client);
    void sendFolderCreationMessage(std::shared_ptr<Folder> f, net::Client *client);
    void sendFileCreationMessage(std::string fid, std::shared_ptr<File> f, net::Client *client);
    bool sendFilePartMessage(std::string fid, std::shared_ptr<File> f, unsigned int nextPartNr, net::Client *client);
    void sendClientHelloMessage(unsigned short listenPort, net::Client *client, unsigned char flags);
    void sendPingMessage(unsigned int plLength, unsigned int seqNumber, net::Client *client);
    void sendFileStatusMessage(std::string fid, std::shared_ptr<File>, net::Client *client);
    void onServerHelloMessage(net::ReadMessage *msg);
    void onFileStatusMessage(net::ReadMessage *msg);
    void onAckMessage(net::ReadMessage *msg);
};