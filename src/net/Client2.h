#pragma once

#include <mutex>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <semaphore.h>
#include "net/AbstractMessage.h"
#include "sec/DiffieHellman.h"
#include "Logger.h"
#include "Consts.h"
#include "Timer.h"
#include "TimerTickable.h"

#define UNLIMITED_PPS 0

namespace net
{
enum ClientState
{
    uc_uninit,
    uc_init
};

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
class Client2 : public TimerTickable
{
  public:
    Client2(std::string hostAddr, unsigned short port, unsigned int maxPPS, sec::DiffieHellman *enc);
    ~Client2();
    ClientState getState();
    void init();
    bool send(net::AbstractMessage *msg);
    bool send(net::Message *msg);
    void onTimerTick(int identifier);

  private:
    ClientState state;
    std::mutex *stateMutex;
    int sockFD;
    unsigned short port;
    std::string hostAddr;
    struct sockaddr_in serverAddressStruct;
    sec::DiffieHellman *enc;
    unsigned int maxPPS;
    unsigned int sendPCount;
    std::mutex *sendPCountMutex;
    Timer* sendPCTimer;
    sem_t maxPPSSema;

    void setState(ClientState state);
    void waitToSend();
};
}