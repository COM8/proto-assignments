#include "Server2.h"

using namespace net;
using namespace std;
using namespace sec;

Server2::Server2(unsigned short port, Queue<ReadMessage> *cpQueue, DiffieHellman *enc)
{
    this->port = port;
    this->cpQueue = cpQueue;
    this->enc = enc;

    this->state = us_stopped;
    this->stateMutex = new mutex();
    this->shouldServerRun = false;
    this->myAddr = {};
    this->sockFD = -1;
    this->serverThread = NULL;
}

Server2::~Server2()
{
    stop();
    if (sockFD > 0)
    {
        close(sockFD);
    }
    delete stateMutex;
}

ServerState Server2::getState()
{
    unique_lock<mutex> mlock(*stateMutex);
    ServerState s = state;
    mlock.unlock();
    return s;
}

void Server2::setState(ServerState state)
{
    unique_lock<mutex> mlock(*stateMutex);

    if (this->state == state)
    {
        mlock.unlock();
        return;
    }

    this->state = state;
    mlock.unlock();

    switch (state)
    {
    case us_started:
        startServerThread();
        break;

    case us_stopped:
        stopServerThread();
        break;

    default:
        break;
    }
}

void Server2::start()
{
    setState(us_started);
}

void Server2::stop()
{
    setState(us_stopped);
}

void Server2::startServerThread()
{
    Logger::info("Startig server on port: " + to_string(port));
    shouldServerRun = true;
    if (serverThread)
    {
        stopServerThread();
    }
    serverThread = new thread(&Server2::serverTask, this);
}

void Server2::stopServerThread()
{
    Logger::info("Stoping server on port: " + to_string(port));
    shouldServerRun = false;
    if (serverThread && serverThread->joinable() && serverThread->get_id() != this_thread::get_id())
    {
        // Push dummy message to wak up server:
        // ToDo ...
        serverThread->join();
    }

    if (serverThread)
    {
        delete serverThread;
    }
    serverThread = NULL;
}

void Server2::serverTask()
{
    if (sockFD > 0)
    {
        Logger::error("UDP server already running! Please stop first!");
    }
    else if ((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        Logger::error("UDP socket creation failed with: " + string(strerror(errno)));
        setState(us_stopped);
    }
    else
    {
        // Fill memory block with 0:
        myAddr = {};

        myAddr.sin_family = AF_INET;
        myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myAddr.sin_port = htons(port);

        if (bind(sockFD, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
        {
            Logger::error("UDP server binding failed with: " + string(strerror(errno)));
            setState(us_stopped);
        }
        else
        {
            // Setup read timeout:
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            if (setsockopt(sockFD, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            {
                Logger::error("UDP server setting read timeout failed with: " + string(strerror(errno)));
                setState(us_stopped);
            }
            else
            {
                Logger::info("Started server on port: " + to_string(port));
                contRead();
            }
        }
    }
    Logger::info("Stopped server on port: " + to_string(port));
}

void Server2::contRead()
{
    int recvlen = -1;
    unsigned char buf[BUF_SIZE];
    struct sockaddr_in remAddr;
    socklen_t addrLen = sizeof(remAddr);
    struct ReadMessage msg = {};

    while (getState() == us_started)
    {
        recvlen = -1;
        do
        {
            recvlen = recvfrom(sockFD, buf, BUF_SIZE, 0, (struct sockaddr *)&remAddr, &addrLen);
        } while (getState() == us_started && recvlen <= 0);

        if (recvlen > 0 && recvlen < BUF_SIZE)
        {
            if (ENABLE_UDP_SERVER_DEBUG_OUTPUT)
            {
                AbstractMessage::printByteArray(buf, recvlen);
            }

            msg.bufferLength = recvlen;
            msg.buffer = buf;
            msg.msgType = buf[0] >> 4;
            // Get sender IP:
            inet_ntop(AF_INET, &(remAddr.sin_addr), msg.senderIp, INET_ADDRSTRLEN);

            // Decrypt message:
            if (enc && enc->isConnectionSecure())
            {
                enc->decrypt(msg.buffer, msg.bufferLength);
            }

            // Insert in consumer producer queue:
            cpQueue->push(msg);
        }
    }
}