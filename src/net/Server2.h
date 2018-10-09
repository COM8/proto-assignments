#pragma once

#include <mutex>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cerrno>
#include <string>
#include <thread>
#include <unistd.h>
#include "../sec/DiffieHellman.h"
#include "../Logger.h"
#include "../Queue.h"
#include "../Consts.h"
#include "AbstractMessage.h"

namespace net
{
enum ServerState
{
    us_stopped,
    us_started
};

// Based on: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
class Server2
{
  public:
    Server2(unsigned short port, Queue<net::ReadMessage> *cpQueue, sec::DiffieHellman *enc);
    ~Server2();
    ServerState getState();
    void start();
    void stop();

  private:
    ServerState state;
    std::mutex *stateMutex;
    int sockFD;
    unsigned short port;
    struct sockaddr_in myAddr;
    std::thread *serverThread;
    bool shouldServerRun;
    Queue<net::ReadMessage> *cpQueue;
    sec::DiffieHellman *enc;

    void setState(ServerState state);
    void serverTask();
    void startServerThread();
    void stopServerThread();
    void contRead();
};
}
