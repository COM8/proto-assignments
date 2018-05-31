#pragma once

#include <thread>
#include <cstdint>
#include <unordered_map>
#include <time.h>
#include <mutex>
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
#include "Logger.h"
#include "FileServerUser.h"
#include "FileServerClient.h"

enum FileServerState
{
    fs_stopped = 0,
    fs_running = 1
};

class FileServer2
{
  public:
    FileServer2(unsigned short port);
    ~FileServer2();

    FileServerState getState();
    void start();
    void stop();

  private:
    FileServerState state;
    std::mutex *stateMutex;
    std::mutex *userMutex;
    net::Server *udpServer;
    unsigned short port;
    Queue<net::ReadMessage> *cpQueue;
    bool shouldConsumerRun;
    std::thread *consumerThread;
    std::unordered_map<std::string, FileServerUser *> users;
    Timer *cleanupTimer;

    FileServerUser *getUser(std::string userName);
    FileServerUser *addUser(std::string userName, std::string password);
    void setState(FileServerState state);
    void consumerTask();
    void startConsumerThread();
    void stopConsumerThread();
    void deleteAllUsers();
    void onClientHelloMessage(net::ReadMessage *msg);
    FileServerClient *findClient(unsigned int clientId);
};