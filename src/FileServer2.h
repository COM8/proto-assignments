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

enum FileServerState
{
    fs_stopped = 0,
    fs_running = 1
};

enum FileServerClientState
{
    fsc_disconnected = 0,
    fsc_clientHello = 1,
    fsc_handshake = 2,
    fsc_connected = 3,
    fsc_ping = 4,
    fsc_error = 5
};

struct FileServerClient
{
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
    net::Server *udpServer;
    unsigned short port;
    Queue<net::ReadMessage> *cpQueue;

    void setState(FileServerState state);
};