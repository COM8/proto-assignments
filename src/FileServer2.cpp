#include "FileServer2.h"

using namespace std;
using namespace net;

FileServer2::FileServer2(unsigned short port)
{
    this->port = port;

    this->stateMutex = new mutex();
    this->cpQueue = new Queue<ReadMessage>();
    this->udpServer = new Server(port, cpQueue);
}

FileServer2::~FileServer2()
{
    delete stateMutex;
    delete udpServer;
    delete cpQueue;
}

FileServerState FileServer2::getState()
{
    unique_lock<mutex> mlock(*stateMutex);
    FileServerState s = state;
    mlock.unlock();
    return s;
}

void FileServer2::setState(FileServerState state)
{
    unique_lock<mutex> mlock(*stateMutex);
    if (this->state == state)
    {
        mlock.unlock();
        return;
    }

    this->state = state;

    switch (state)
    {
    case fs_running:
        break;

    case fs_stopped:
        break;

    default:
        Logger::warn("Invalid FileServer2 state: " + to_string(state));
        break;
    }

    mlock.unlock();
}