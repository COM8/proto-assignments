#include "Client2.h"

using namespace net;
using namespace sec;
using namespace std;

Client2::Client2(string hostAddr, unsigned short port, unsigned int maxPPS, DiffieHellman *enc)
{
    this->hostAddr = hostAddr;
    this->port = port;
    this->maxPPS = maxPPS;
    this->enc = enc;

    this->stateMutex = new mutex();
    this->state = uc_uninit;
    this->sockFD = -1;
    srand(time(NULL));
    this->sendPCount = 0;
    this->sendPCTimer = new Timer(true, 1000, this, 1);
    this->sendPCTimer->start();
    sem_init(&maxPPSSema, 0, maxPPS);
    this->sendPCountMutex = new mutex();
}

Client2::~Client2()
{
    sendPCTimer->stop();
    sem_destroy(&maxPPSSema);
    delete stateMutex;
    delete sendPCTimer;
    delete sendPCountMutex;
}

ClientState Client2::getState()
{
    unique_lock<mutex> mlock(*stateMutex);
    ClientState s = state;
    mlock.unlock();
    return s;
}

void Client2::setState(ClientState state)
{
    unique_lock<mutex> mlock(*stateMutex);
    if (this->state != state)
    {
        this->state = state;
    }
    mlock.unlock();
}

void Client2::init()
{
    if (getState() != uc_uninit)
    {
        Logger::warn("Unable to init UDP client! Client is already initialized.");
        return;
    }

    if ((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        Logger::error("UDP socket creation failed with: " + string(strerror(errno)));
    }
    else
    {
        serverAddressStruct = {};
        serverAddressStruct.sin_family = AF_INET;
        serverAddressStruct.sin_port = htons(port);

        // Look up the address of the server given its name:
        struct hostent *hp = {};
        hp = gethostbyname(hostAddr.c_str());
        if (!hp)
        {
            Logger::error("UDP client unable to look up host: " + hostAddr);
        }
        else
        {
            // Add host address to the address structure:
            memcpy((void *)&serverAddressStruct.sin_addr, hp->h_addr_list[0], hp->h_length);
            setState(uc_init);
        }
    }
}

bool Client2::send(AbstractMessage *msg)
{
    Message msgStruct{};
    msg->createBuffer(&msgStruct);
    if (msgStruct.bufferLength <= 0)
    {
        AbstractMessage::printByte(msg->getType());
        Logger::error("11-------------------------------------------- for type: ");
        msg->createBuffer(&msgStruct);
    }
    bool result = send(&msgStruct);

    // Cleanup:
    delete[] msgStruct.buffer;

    return result;
}

bool Client2::send(Message *msg)
{
    if (getState() != uc_init)
    {
        Logger::error("Unable to send messge! Initialization required.");
        return false;
    }

    // Print message:
    if (ENABLE_UDP_CLIENT_DEBUG_OUTPUT)
    {
        AbstractMessage::printMessage(msg);
    }

    bool success = true;

    // Message drop chance:
    if (rand() % 100 + 1 <= CLIENT_MESSAGE_DROP_CHANCE)
    {
        Logger::info("Droped message!");
    }
    else
    {
        // Encrypt message:
        if (enc && enc->isConnectionSecure())
        {
            enc->encrypt(msg->buffer, msg->bufferLength);
        }

        // Wait for a slot to send the package:
        waitToSend();

        if (sendto(sockFD, msg->buffer, msg->bufferLength, 0, (struct sockaddr *)&serverAddressStruct, sizeof(serverAddressStruct)) < 0)
        {
            Logger::error("UDP client failed to send message to: " + hostAddr + " on port: " + to_string(port) + " with error: " + string(strerror(errno)) + " lenght: " + to_string(msg->bufferLength));
            success = false;
        }
    }
    return success;
}

void Client2::waitToSend()
{
    if (maxPPS <= UNLIMITED_PPS)
    {
        return;
    }
    Logger::debug("Requirering lock...");
    sem_wait(&maxPPSSema);
    Logger::debug("Received lock.");
    unique_lock<mutex> mlock(*stateMutex);
    sendPCount++;
    mlock.unlock();
}

void Client2::onTimerTick(int identifier)
{
    if (identifier == 1)
    {
        unique_lock<mutex> mlock(*stateMutex);
        while (sendPCount > 0)
        {
            sendPCount--;
            sem_post(&maxPPSSema);
        }
        mlock.unlock();
    }
}