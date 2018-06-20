#pragma once

#include <mutex>
#include <cstdint>
#include <time.h>
#include <string>
#include "net/Client2.h"
#include "net/Server2.h"
#include "Filesystem.h"
#include "ClientsToDo.h"
#include "AbstractClient.h"

enum FileServerClientState
{
  fsc_disconnected = 0,
  fsc_clientHello = 1,
  fsc_awaitClientAuth = 2,
  fsc_awaitServerAuthAck = 3,
  fsc_connected = 4,
  fsc_sendingChanges = 5,
  fsc_awaitingAck = 6,
  fsc_ping = 7,
  fsc_error = 8
};

struct ToDoHelper
{
  bool sendCreationMsg;
  TodoEntry *curToDo;
  ClientToDo *clientToDo;

  ToDoHelper()
  {
    curToDo = NULL;
    clientToDo = NULL;
    sendCreationMsg = false;
  }

  void loadNext()
  {
    sendCreationMsg = false;
    if (clientToDo)
    {
      curToDo = clientToDo->getNext();
    }
    else
    {
      curToDo = NULL;
    }
  }
};

class FileServerClient : public AbstractClient
{

public:
  const unsigned short PORT_LOCAL;
  const unsigned short PORT_REMOTE;
  const char *IP_REMOTE;
  time_t lastMessageTime;

  FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, unsigned int maxPPS, class FileServerUser *user);
  ~FileServerClient();
  void disconnect();
  FileServerClientState getState();
  void setDeclined(unsigned char flags);
  void setAccepted(unsigned long prime, unsigned long primRoot, unsigned long pubKey);
  void onTimerTick(int identifier);
  void onMessageReceived(net::ReadMessage *msg);

private:
  class FileServerUser *user;
  FileServerClientState state;
  std::mutex stateMutex;
  net::Client2 *udpClient;
  net::Server2 *udpServer;
  std::string curFID;
  unsigned int lastFIDPartNumber;
  std::uint64_t curFIDLength;
  unsigned int maxPPS;
  ToDoHelper toDoHelper;

  void setState(FileServerClientState state);
  void onDisconnectedOrErrorState();
  void onPingMessage(net::ReadMessage *msg);
  void onAckMessage(net::ReadMessage *msg);
  void onFileCreationMessage(net::ReadMessage *msg);
  void onFileTransferMessage(net::ReadMessage *msg);
  void onTransferEndedMessage(net::ReadMessage *msg);
  void onFileStatusMessage(net::ReadMessage *msg);
  void onAuthRequestMessage(net::ReadMessage *msg);
  void sendNextClientToDo();
};