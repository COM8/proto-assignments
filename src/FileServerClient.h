#pragma once

#include <mutex>
#include <cstdint>
#include <time.h>
#include "net/Client.h"
#include "net/Server.h"
#include "Queue.h"
#include "Filesystem.h"
#include "net/AbstractMessage.h"
#include "net/ServerHelloMessage.h"
#include "net/FileCreationMessage.h"
#include "net/FileTransferMessage.h"
#include "net/TransferEndedMessage.h"
#include "net/FileStatusMessage.h"
#include "net/PingMessage.h"
#include "net/AckMessage.h"

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

  FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, class FileServerUser *user);
  ~FileServerClient();
  void disconnect();
  FileServerClientState getState();
  void setDeclined(unsigned char flags);
  void setAccepted(unsigned char flags);

private:
  const class FileServerUser *USER;
  FileServerClientState state;
  std::mutex stateMutex;
  net::Client *udpClient;
  net::Server *udpServer;
  Queue<net::ReadMessage> *cpQueue;
  bool shouldConsumerRun;
  std::thread *consumerThread;
  time_t lastMessageTime;
  std::string curFID;
  unsigned int lastFIDPartNumber;
  std::uint64_t curFIDLength;

  void setState(FileServerClientState state);
  void consumerTask();
  void startConsumerThread();
  void stopConsumerThread();
  void sendServerHelloMessage(unsigned char flags);
  void sendAckMessage(unsigned int seqNumber);
  void sendFileStatusAnswerMessage(unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID);
  void onPingMessage(net::ReadMessage *msg);
  void onAckMessage(net::ReadMessage *msg);
  void onFileCreationMessage(net::ReadMessage *msg);
  void onFileTransferMessage(net::ReadMessage *msg);
  void onTransferEndedMessage(net::ReadMessage *msg);
  void onFileStatusMessage(net::ReadMessage *msg);
};