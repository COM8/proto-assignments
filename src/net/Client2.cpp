#include "Client2.h"

using namespace net;
using namespace sec;
using namespace std;

Client2::Client2(string hostAddr, unsigned short port, DiffieHellman *enc)
{
    this->hostAddr = hostAddr;
    this->enc = enc;
    this->port = port;

    this->stateMutex = new mutex();
    this->state = uc_uninit;
    this->sockFD = -1;
    srand(time(NULL));
}

Client2::~Client2()
{
    delete stateMutex;
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
    Message msgStruct = {};
    msg->createBuffer(&msgStruct);
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

        if (sendto(sockFD, msg->buffer, msg->bufferLength, 0, (struct sockaddr *)&serverAddressStruct, sizeof(serverAddressStruct)) < 0)
        {
            Logger::error("UDP client failed to send message to: " + hostAddr + " on port: " + to_string(port) + " with error: " + string(strerror(errno)) + " lenght: " + to_string(msg->bufferLength));
            success = false;
        }
    }
    return success;
}