#pragma once

#include <mutex>
#include <cstdint>
#include <time.h>
#include "net/Client2.h"
#include "net/Server2.h"
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
#include "sec/DiffieHellman.h"
#include "Timer.h"
#include "TimerTickable.h"
#include "Consts.h"

enum FileServerClientState
{
  fsc_disconnected = 0,
  fsc_clientHello = 1,
  fsc_awaitHandshAck = 2,
  fsc_connected = 3,
  fsc_sendingChanges = 4,
  fsc_awaitingAck = 5,
  fsc_ping = 6,
  fsc_error = 7
};

class FileServerClient : TimerTickable
{

public:
  const unsigned int CLIENT_ID;
  const unsigned short PORT_LOCAL;
  const unsigned short PORT_REMOTE;
  const char *IP_REMOTE;
  time_t lastMessageTime;

  FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, unsigned int maxPPS, class FileServerUser *user);
  ~FileServerClient();
  void disconnect();
  FileServerClientState getState();
  void setDeclined(unsigned char flags);
  void setAccepted(unsigned char flags, unsigned long prime, unsigned long primRoot, unsigned long pubKey);
  void onTimerTick(int identifier);

private:
  class FileServerUser *user;
  FileServerClientState state;
  std::mutex stateMutex;
  std::mutex *seqNumberMutex;
  unsigned int seqNumber;
  net::Client2 *udpClient;
  net::Server2 *udpServer;
  Queue<net::ReadMessage> *cpQueue;
  SendMessageQueue *sendMessageQueue;
  bool shouldConsumerRun;
  std::thread *consumerThread;
  std::string curFID;
  unsigned int lastFIDPartNumber;
  std::uint64_t curFIDLength;
  sec::DiffieHellman *enc;
  Timer *tTimer;
  unsigned int maxPPS;

  void setState(FileServerClientState state);
  unsigned int getNextSeqNumber();
  void onDisconnectedOrErrorState();
  void consumerTask();
  void startConsumerThread();
  void stopConsumerThread();
  void sendServerHelloMessage(unsigned char flags, unsigned long pubKey);
  void sendAckMessage(unsigned int seqNumber);
  void sendFileStatusAnswerMessage(unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID);
  void sendPingMessage(unsigned int plLength, unsigned int seqNumber);
  void onPingMessage(net::ReadMessage *msg);
  void onAckMessage(net::ReadMessage *msg);
  void onFileCreationMessage(net::ReadMessage *msg);
  void onFileTransferMessage(net::ReadMessage *msg);
  void onTransferEndedMessage(net::ReadMessage *msg);
  void onFileStatusMessage(net::ReadMessage *msg);
};