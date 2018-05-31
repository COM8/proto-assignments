#pragma once

#include <mutex>
#include <cstdint>
#include <time.h>
#include "net/Client.h"
#include "net/Server.h"
#include "Queue.h"
#include "Filesystem.h"
#include "FileServerUser.h"
#include "net/AbstractMessage.h"
#include "net/ServerHelloMessage.h"

enum FileServerClientState
{
  fsc_disconnected = 0,
  fsc_clientHello = 1,
  fsc_handshake = 2,
  fsc_connected = 3,
  fsc_sendingChanges = 4,
  fsc_awaitingAck = 5,
  fsc_ping = 6
};

class FileServerClient
{
public:
  const unsigned int CLIENT_ID;
  const unsigned short PORT_LOCAL;
  const unsigned short PORT_REMOTE;
  const char *IP_REMOTE;

  FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, FileServerUser *user);
  ~FileServerClient();
  void disconnect();
  FileServerClientState getState();
  void setDeclined(unsigned char flags);
  void setAccepted(unsigned char flags);

private:
  const FileServerUser *USER;
  FileServerClientState state;
  std::mutex stateMutex;
  net::Client *udpClient;
  net::Server *udpServer;
  Queue<net::ReadMessage> *cpQueue;
  bool shouldConsumerRun;
  std::thread *consumerThread;
  time_t lastMessageTime;

  void setState(FileServerClientState state);
  void consumerTask();
  void startConsumerThread();
  void stopConsumerThread();
  void sendServerHelloMessage(unsigned char flags);
};