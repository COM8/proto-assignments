#pragma once

#include <iostream>
#include <mutex>
#include <list>
#include <unistd.h>
#include "Filesystem.h"
#include "net/Client2.h"
#include "net/Server2.h"
#include "net/ClientHelloMessage.h"
#include "net/ServerHelloMessage.h"
#include "net/PingMessage.h"
#include "net/AckMessage.h"
#include "net/TransferEndedMessage.h"
#include "net/FileCreationMessage.h"
#include "net/FileTransferMessage.h"
#include "net/FileStatusMessage.h"
#include "net/AuthRequestMessage.h"
#include "net/AuthResultMessage.h"
#include "Queue.h"
#include "WorkingSet.h"
#include "Timer.h"
#include "TimerTickable.h"
#include "Logger.h"
#include "sec/DiffieHellman.h"
#include "Consts.h"

enum FileClient2State
{
    disconnected = 0,
    sendClientHello = 1,
    clientAuth = 2,
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
    FileClient2(std::string serverAddress, unsigned short serverPort, std::string username, std::string clientPassword, FilesystemClient *fS);
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
    std::string username;
    net::Client2 *client;
    net::Client2 *uploadClient;
    net::Server2 *server;
    Queue<net::ReadMessage> *cpQueue;
    SendMessageQueue *sendMessageQueue;
    bool reconnect;
    unsigned int msgTimeoutCount;
	Timer *tTimer;
    WorkingSet *curWorkingSet;
    bool transferFinished;
    sec::DiffieHellman *enc;

    void setState(FileClient2State state);
    void startSendingFS();
    unsigned int getRandomClientId();
    unsigned int getNextSeqNumber();
    void startHelloThread();
    void stopHelloThread();
    void helloTask(unsigned short listenPort, bool reconnecting, net::Client2 *client);
    void stopConsumerThread();
    void startConsumerThread();
    void consumerTask();
    void sendNextFilePart();
    void sendFolderDeletionMessage(std::string folder, net::Client2 *client);
    void sendFileDeletionMessage(std::string file, net::Client2 *client);
    void sendTransferEndedMessage(unsigned char flags, net::Client2 *client);
    void sendFolderCreationMessage(std::shared_ptr<Folder> f, net::Client2 *client);
    void sendFileCreationMessage(std::string fid, std::shared_ptr<File> f, net::Client2 *client);
    bool sendFilePartMessage(std::string fid, std::shared_ptr<File> f, unsigned int nextPartNr, net::Client2 *client);
    void sendClientHelloMessage(unsigned short listenPort, unsigned char flags, std::string username, net::Client2 *client);
    void sendPingMessage(unsigned int plLength, unsigned int seqNumber, net::Client2 *client);
    void sendFileStatusMessage(std::string fid, std::shared_ptr<File>, net::Client2 *client);
    void sendAckMessage(unsigned int seqNumber, net::Client2 *client);
    void sendAuthRequestMessage(unsigned int seqNumber, net::Client2 *client);
    void onServerHelloMessage(net::ReadMessage *msg);
    void onFileStatusMessage(net::ReadMessage *msg);
    void onAckMessage(net::ReadMessage *msg);
    void onPingMessage(net::ReadMessage *msg);
    void onAuthResultMessage(net::ReadMessage *msg);
};