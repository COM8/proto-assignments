#include "FileClient2.h"

using namespace net;
using namespace std;

FileClient2::FileClient2(string serverAddress, unsigned short serverPort, unsigned int clientId, string clientPassword, FilesystemClient *fS)
{
    this->serverAddress = serverAddress;
    this->serverPort = serverPort;
    this->fS = fS;

    this->state = disconnected;
    this->stateMutex = new mutex();
    this->seqNumberMutex = new mutex();
    this->seqNumber = 0;
    this->helloThread = NULL;
    this->shouldHelloRun = false;
    this->shouldConsumerRun = false;
    this->consumerThread = NULL;
    this->client = NULL;
    this->uploadClient = NULL;
    this->server = NULL;
    this->cpQueue = new Queue<ReadMessage>();
    this->sendMessageQueue = new SendMessageQueue();
    this->reconnect = false;
    this->msgTimeoutCount = 0;
    this->uploadPort = 0;
    this->tTimer = new Timer(true, 1000, this, 1);
    this->clientId = clientId;
    this->clientPassword = clientPassword;
}

FileClient2::~FileClient2()
{
    stopHelloThread();
    delete stateMutex;
    delete seqNumberMutex;
    delete client;
    delete uploadClient;
    delete server;
    delete cpQueue;
    delete sendMessageQueue;
}

unsigned int FileClient2::getNextSeqNumber()
{
    unique_lock<mutex> mlock(*seqNumberMutex);
    unsigned int i = seqNumber++;
    mlock.unlock();
    return i;
}

FileClient2State FileClient2::getState()
{
    unique_lock<mutex> mlock(*stateMutex);
    FileClient2State s = state;
    mlock.unlock();
    return s;
}

void FileClient2::setState(FileClient2State state)
{
    unique_lock<mutex> mlock(*stateMutex);
    if (this->state == state)
    {
        mlock.unlock();
        return;
    }
    Logger::debug("[FileClient2]: " + to_string(this->state) + " -> " + to_string(state));
    this->state = state;

    FileClient2State s = state;
    mlock.unlock();

    switch (s)
    {
    case disconnected:
        reconnect = false;
        tTimer->stop();
        stopHelloThread();
        stopConsumerThread();
        sendMessageQueue->clear();
        break;

    case sendClientHello:
        msgTimeoutCount = 0;
        startConsumerThread();
        startHelloThread();
        break;

    case handshake:
        Logger::info("Connected to: " + serverAddress + " on port: " + to_string(uploadPort));
        setState(connected);
        break;

    case connected:
        tTimer->start();
        break;

    case ping:
        tTimer->stop();
        sendPingMessage(0, getNextSeqNumber(), uploadClient);
        break;

    case reqestedFileStatus:
        tTimer->reset();
        break;

    case sendingFS:
        tTimer->stop();
        break;

    case awaitingAck:
        tTimer->start();
        break;

    case resending:
        tTimer->stop();
        break;
    }
}

void FileClient2::connect()
{
    FileClient2State state = getState();
    if (state == disconnected)
    {
        listenPort = 2000 + rand() % 63000; // Pick random lisen port
        client = new Client(serverAddress, serverPort);
        server = new Server(listenPort, cpQueue);
        server->start();

        setState(sendClientHello);
    }
    else
    {
        Logger::error("FileClient2::connect invalid state: " + to_string(state));
    }
}

void FileClient2::startSendingFS()
{
    FileClient2State state = getState();
    if (state == connected)
    {
        setState(sendingFS);
    }
    else
    {
        Logger::error("FileClient2::startSendingFS invalid state: " + to_string(state));
    }
}

void FileClient2::startHelloThread()
{
    shouldHelloRun = true;
    helloThread = new thread(&FileClient2::helloTask, this, listenPort, reconnect, client);
}

void FileClient2::stopHelloThread()
{
    shouldHelloRun = false;
    if (helloThread && helloThread->joinable() && helloThread->get_id() != this_thread::get_id())
    {
        helloThread->join();
        delete helloThread;
    }
}

void FileClient2::helloTask(unsigned short listenPort, bool reconnecting, Client *client)
{
    Logger::debug("Started hello thread.");
    while (getState() == sendClientHello && shouldHelloRun)
    {
        unsigned char flags = 0b0001;
        if (reconnecting)
        {
            flags |= 0b0010;
        }
        sendClientHelloMessage(listenPort, client, flags);
        sleep(1); // Sleep for 1 second
    }
    Logger::debug("Stopped hello thread.");
}

void FileClient2::sendClientHelloMessage(unsigned short listenPort, Client *client, unsigned char flags)
{
    ClientHelloMessage *msg = new ClientHelloMessage(listenPort, clientId, flags);
    client->send(msg);
}

void FileClient2::startConsumerThread()
{
    shouldConsumerRun = true;
    consumerThread = new thread(&FileClient2::consumerTask, this);
}

void FileClient2::stopConsumerThread()
{
    shouldConsumerRun = false;
    if (consumerThread && consumerThread->joinable() && consumerThread->get_id() != this_thread::get_id())
    {
        ReadMessage *msg = new ReadMessage();
        msg->msgType = 0xff;
        cpQueue->push(*msg); // Push dummy message to wake the consumer up
        consumerThread->join();
    }
}

void FileClient2::consumerTask()
{
    Logger::debug("Stopped consumer thread.");
    while (shouldConsumerRun)
    {
        ReadMessage msg = cpQueue->pop();
        if (!shouldConsumerRun)
        {
            break;
        }

        switch (msg.msgType)
        {
        case 2:
            onServerHelloMessage(&msg);
            break;

        case 4:
            // onFileStatusMessage(&msg);
            break;

        case 5:
            onAckMessage(&msg);
            break;

        case 7:
            // onTransferEndedMessage(&msg);
            break;

        default:
            Logger::warn("Unknown message type received: " + to_string(msg.msgType));
            break;
        }
    }
    Logger::debug("Stopend consumer thread.");
}

void FileClient2::onServerHelloMessage(ReadMessage *msg)
{
    if (getState() == sendClientHello)
    {
        // Check if checksum is valid:
        if (!AbstractMessage::isChecksumValid(msg, ServerHelloMessage::CHECKSUM_OFFSET_BITS))
        {
            return;
        }

        unsigned char flags = ServerHelloMessage::getFlagsFromMessage(msg->buffer);
        if ((flags & 0b0001) != 1)
        {
            Logger::error("Client not accepted: " + to_string(flags));
            setState(disconnected);
            return;
        }

        uploadPort = ServerHelloMessage::getPortFromMessage(msg->buffer);
        uploadClient = new Client(serverAddress, uploadPort);

        setState(handshake);
    }
    else
    {
        Logger::error("FileClient2::onServerHelloMessage invalid state: " + to_string(state));
    }
}

void FileClient2::disconnect()
{
    setState(disconnected);
}

void FileClient2::onTimerTick(int identifier)
{
    FileClient2State state = getState();

    switch (state)
    {
    case connected:
        setState(ping);
        break;

    case ping:
        sendMessageQueue->clear();
        msgTimeoutCount++;
        Logger::warn("Ping timeout " + to_string(msgTimeoutCount));
        if (msgTimeoutCount > 3)
        {
            Logger::warn("Resending ping");
            sendPingMessage(0, getNextSeqNumber(), uploadClient);
            setState(connected);
        }
        else
        {
            Logger::error("Server timed out - reconnecting...");
            disconnect();
            connect();
        }
        break;
    }
}

void FileClient2::sendPingMessage(unsigned int plLength, unsigned int seqNumber, Client *client)
{
    PingMessage *msg = new PingMessage(plLength, seqNumber, clientId);
    sendMessageQueue->pushSendMessage(seqNumber, msg);

    client->send(msg);
    delete msg;
    Logger::debug("Ping");
}

void FileClient2::onAckMessage(ReadMessage *msg)
{
    // Check if checksum is valid:
    if (!AbstractMessage::isChecksumValid(msg, AckMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    unsigned int seqNumber = AckMessage::getSeqNumberFromMessage(msg->buffer);
    if (!sendMessageQueue->onSequenceNumberAck(seqNumber))
    {
        Logger::error("Unable to ACK sequence number: " + to_string(seqNumber) + ". Sequence number was not found!");
    }

    switch (getState())
    {
    case awaitingAck:
        setState(sendingFS);
        break;

    case ping:
        Logger::debug("Pong");
        setState(connected);
        break;
    }
}