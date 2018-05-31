#include "FileServerClient.h"

using namespace std;
using namespace net;

FileServerClient::FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, FileServerUser *user) : CLIENT_ID(clientId),
                                                                                                                                                       PORT_LOCAL(portLocal),
                                                                                                                                                       PORT_REMOTE(portRemote),
                                                                                                                                                       IP_REMOTE(ipRemote),
                                                                                                                                                       USER(user)
{
    this->consumerThread = NULL;
    this->shouldConsumerRun = false;
    this->state = fsc_disconnected;
    this->cpQueue = new Queue<ReadMessage>();
    this->udpServer = new Server(PORT_LOCAL, cpQueue);
    this->udpClient = new Client(IP_REMOTE, PORT_REMOTE);
    this->lastMessageTime = time(NULL);
}

FileServerClient::~FileServerClient()
{
    disconnect();
    delete[] IP_REMOTE;
    delete cpQueue;
    delete udpClient;
    delete udpServer;
}

void FileServerClient::disconnect()
{
    setState(fsc_disconnected);
}

void FileServerClient::setDeclined(unsigned char flags)
{
    sendServerHelloMessage(flags);
    setState(fsc_disconnected);
}
void FileServerClient::setAccepted(unsigned char flags)
{
    sendServerHelloMessage(flags);
    setState(fsc_handshake);
}

FileServerClientState FileServerClient::getState()
{
    unique_lock<mutex> mlock(stateMutex);
    FileServerClientState s = state;
    mlock.unlock();
    return s;
}

void FileServerClient::setState(FileServerClientState state)
{
    unique_lock<mutex> mlock(stateMutex);
    if (this->state == state)
    {
        mlock.unlock();
        return;
    }

    this->state = state;
    mlock.unlock();

    switch (state)
    {
    case fsc_disconnected:
        stopConsumerThread();
        cpQueue->clear();
        udpServer->stop();
        break;

    case fsc_clientHello:
        udpServer->start();
        break;

    case fsc_handshake:
        break;

    case fsc_connected:
        break;

    case fsc_sendingChanges:
        break;

    case fsc_awaitingAck:
        break;

    case fsc_ping:
        break;

    default:
        break;
    }
}

void FileServerClient::startConsumerThread()
{
    shouldConsumerRun = true;
    if (consumerThread)
    {
        stopConsumerThread();
    }
    consumerThread = new thread(&FileServerClient::consumerTask, this);
}

void FileServerClient::stopConsumerThread()
{
    shouldConsumerRun = false;
    if (consumerThread && consumerThread->joinable() && consumerThread->get_id() != this_thread::get_id())
    {
        ReadMessage msg = ReadMessage();
        msg.msgType = 0xff;
        cpQueue->push(msg); // Push dummy message to wake up the consumer thread
        consumerThread->join();
    }

    if (consumerThread)
    {
        delete consumerThread;
    }
    consumerThread = NULL;
}

void FileServerClient::consumerTask()
{
    while (shouldConsumerRun)
    {
        ReadMessage msg = cpQueue->pop();
        if (!shouldConsumerRun)
        {
            return;
        }

        switch (msg.msgType)
        {
        case 1:
            // onClientHelloMessage(&msg);
            break;

        case 2:
            // onFileCreationMessage(&msg);
            break;

        case 3:
            // onFileTransferMessage(&msg);
            break;

        case 4:
            // onFileStatusMessage(&msg);
            break;

        case 5:
            // onAckMessage(&msg);
            break;

        case 6:
            // onPingMessage(&msg);
            break;

        case 7:
            // onTransferEndedMessage(&msg);
            break;

        default:
            cerr << "Unknown message type received: " << msg.msgType << endl;
            break;
        }
    }
}

void FileServerClient::sendServerHelloMessage(unsigned char flags)
{
    ServerHelloMessage *msg = new ServerHelloMessage(PORT_LOCAL, CLIENT_ID, flags);
    udpClient->send(msg);
}