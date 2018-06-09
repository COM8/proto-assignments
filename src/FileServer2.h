#pragma once

#include <thread>
#include <cstdint>
#include <unordered_map>
#include <time.h>
#include <mutex>
#include "net/Server2.h"
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
#include "UserStore.h"

enum FileServerState
{
  fs_stopped = 0,
  fs_running = 1
};

class FileServer2 : public TimerTickable
{
public:
  FileServer2(unsigned short port, unsigned int maxPPS);
  ~FileServer2();

  FileServerState getState();
  void start();
  void stop();

private:
  FileServerState state;
  std::mutex *stateMutex;
  std::mutex *userMutex;
  net::Server2 *udpServer;
  unsigned short port;
  Queue<net::ReadMessage> *cpQueue;
  bool shouldConsumerRun;
  std::thread *consumerThread;
  std::unordered_map<std::string, FileServerUser *> users;
  Timer *cleanupTimer;
  unsigned int maxPPS;

  FileServerUser *getUser(std::string userName);
  FileServerUser *addUser(const User *u);
  void setState(FileServerState state);
  void consumerTask();
  void startConsumerThread();
  void stopConsumerThread();
  void deleteAllUsers();
  void onClientHelloMessage(net::ReadMessage *msg);
  void onTimerTick(int identifier);
  FileServerClient *findClient(unsigned int clientId);
};