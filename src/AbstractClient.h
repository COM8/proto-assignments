#pragma once

#include <string>
#include <mutex>
#include "Timer.h"
#include "TimerTickable.h"
#include "Queue.h"
#include "net/Client2.h"
#include "Logger.h"
#include "Consts.h"
#include "net/AbstractMessage.h"
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
#include "Filesystem.h"

class AbstractClient : public TimerTickable
{
public:
  unsigned int clientId;

  AbstractClient(unsigned int clientId);
  ~AbstractClient();

protected:
  Queue<net::ReadMessage> *cpQueue;
  SendMessageQueue *sendMessageQueue;
  Timer *tTimer;
  sec::DiffieHellman *enc;

  unsigned int getNextSeqNumber();
  void stopConsumerThread();
  void startConsumerThread();
  void consumerTask();
  virtual void onMessageReceived(net::ReadMessage *msg){};
  void sendFolderDeletionMessage(std::string folder, net::Client2 *client);
  void sendFileDeletionMessage(std::string file, net::Client2 *client);
  void sendTransferEndedMessage(unsigned char flags, net::Client2 *client);
  void sendFolderCreationMessage(std::shared_ptr<Folder> f, net::Client2 *client);
  void sendFileCreationMessage(std::string fid, unsigned char *hash, net::Client2 *client);
  void sendFileCreationMessage(std::string fid, unsigned char *hash, net::FileType fileType, net::Client2 *client);
  void sendFileTransferMessage(unsigned char flags, unsigned int fidPartNumber, uint64_t contentLength, unsigned char *content, std::string fid, net::Client2 *client);
  void sendClientHelloMessage(unsigned short listenPort, unsigned char flags, std::string username, net::Client2 *client);
  void sendPingMessage(unsigned int plLength, unsigned int seqNumber, net::Client2 *client);
  void sendFileStatusMessage(std::string fid, std::shared_ptr<File>, net::Client2 *client);
  void sendAckMessage(unsigned int seqNumber, net::Client2 *client);
  void sendAuthRequestMessage(unsigned int seqNumber, std::string password, net::Client2 *client);
  void sendAuthResultMessage(unsigned int seqNumber, unsigned char flags, net::Client2 *client);
  void sendServerHelloMessage(unsigned short portLocal, unsigned char flags, unsigned long pubKey, net::Client2 *client);
  void sendFileStatusAnswerMessage(unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID, net::Client2 *client);

private:
  std::thread consumerThread;
  bool shouldConsumerRun;
  std::mutex *seqNumberMutex;
  unsigned int seqNumber;
};