#include "FileServerClient.h"
#include "FileServerUser.h"

using namespace std;
using namespace net;
using namespace sec;

FileServerClient::FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, unsigned int maxPPS, FileServerUser *user) : CLIENT_ID(clientId),
                                                                                                                                                                            PORT_LOCAL(portLocal),
                                                                                                                                                                            PORT_REMOTE(portRemote),
                                                                                                                                                                            IP_REMOTE(ipRemote)
{
    this->user = user;
    this->maxPPS = maxPPS;

    this->shouldConsumerRun = false;
    this->state = fsc_disconnected;
    this->cpQueue = new Queue<ReadMessage>();
    this->enc = NULL;
    this->udpServer = new Server2(PORT_LOCAL, cpQueue, enc);
    this->udpClient = new Client2(IP_REMOTE, PORT_REMOTE, maxPPS, enc);
    this->udpClient->init();
    this->lastMessageTime = time(NULL);
    this->curFID = "";
    this->curFIDLength = 0;
    this->lastFIDPartNumber = 0;
    this->state = fsc_disconnected;
    this->tTimer = new Timer(true, 1000, this, 1);
    this->sendMessageQueue = new SendMessageQueue();
    this->seqNumber = 0;
    this->seqNumberMutex = new mutex();
}

FileServerClient::~FileServerClient()
{
    setState(fsc_error);
    delete tTimer;
    delete cpQueue;
    delete udpClient;
    delete udpServer;
    delete sendMessageQueue;
    delete seqNumberMutex;
}

unsigned int FileServerClient::getNextSeqNumber()
{
    unique_lock<mutex> mlock(*seqNumberMutex);
    unsigned int i = seqNumber++;
    mlock.unlock();
    return i;
}

void FileServerClient::onTimerTick(int identifier)
{
    if (identifier == 1)
    {
        switch (getState())
        {
        case fsc_connected:
            setState(fsc_ping);
            break;

        case fsc_ping:
        case fsc_awaitingAck:
        case fsc_awaitClientAuth:
        case fsc_awaitServerAuthAck:
            list<struct SendMessage> *msgs = new list<struct SendMessage>();
            sendMessageQueue->popNotAckedMessages(MAX_ACK_TIME_IN_S, msgs);

            for (struct SendMessage msg : *msgs)
            {
                if (msg.sendCount > MAX_MESSAGE_TIMEOUT_COUNT)
                {
                    Logger::error("Failed to send messge " + to_string(msg.sendCount) + " times - reconnecting!");
                    setState(fsc_error);
                }
                else
                {
                    // Resend message:
                    udpClient->send(msg.msg);
                    msg.sendCount++;
                    msg.sendTime = time(NULL);
                    sendMessageQueue->push(msg);
                    Logger::info("Resending message.");
                }
            }
            delete msgs;
            break;
        }
    }
}

void FileServerClient::onDisconnectedOrErrorState()
{
    tTimer->stop();
    stopConsumerThread();
    cpQueue->clear();
    udpServer->stop();
    sendMessageQueue->clear();
}

void FileServerClient::disconnect()
{
    setState(fsc_disconnected);
}

void FileServerClient::setDeclined(unsigned char flags)
{
    sendServerHelloMessage(flags, -1);
    setState(fsc_disconnected);
}
void FileServerClient::setAccepted(unsigned long prime, unsigned long primRoot, unsigned long pubKey)
{
    // Setup encryption:
    if (enc)
    {
        delete enc;
        enc = NULL;
    }
    enc = new DiffieHellman();
    enc->onServerReceive(prime, primRoot, pubKey);

    setState(fsc_clientHello);
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
    Logger::debug("[FileServerClient " + to_string(CLIENT_ID) + "]: " + to_string(this->state) + " -> " + to_string(state));
    this->state = state;
    mlock.unlock();

    switch (state)
    {
    case fsc_disconnected:
        onDisconnectedOrErrorState();
        break;

    case fsc_clientHello:
        startConsumerThread();
        udpServer->start();
        sendServerHelloMessage(0b0001, enc->getPubKey());
        setState(fsc_awaitClientAuth);
        break;

    case fsc_awaitClientAuth:
        tTimer->start();
        break;

    case fsc_awaitServerAuthAck:
        tTimer->reset();
        break;

    case fsc_connected:
        tTimer->reset();
        break;

    case fsc_sendingChanges:
        tTimer->stop();
        break;

    case fsc_awaitingAck:
        tTimer->start();
        break;

    case fsc_ping:
        tTimer->start();
        break;

    case fsc_error:
        onDisconnectedOrErrorState();
        break;

    default:
        Logger::warn("Invalid FileServerClient state: " + to_string(state));
        break;
    }
}

void FileServerClient::startConsumerThread()
{
    shouldConsumerRun = true;
    consumerThread = thread(&FileServerClient::consumerTask, this);
}

void FileServerClient::stopConsumerThread()
{
    shouldConsumerRun = false;
    if (consumerThread.joinable() && consumerThread.get_id() != this_thread::get_id())
    {
        ReadMessage msg = ReadMessage();
        msg.msgType = 0xff;
        cpQueue->push(msg); // Push dummy message to wake up the consumer thread
        consumerThread.join();
    }
}

void FileServerClient::consumerTask()
{
    Logger::debug("Started server client consumer thread.");
    while (shouldConsumerRun)
    {
        ReadMessage msg = cpQueue->pop();
        if (!shouldConsumerRun)
        {
            return;
        }

        lastMessageTime = time(NULL);

        switch (msg.msgType)
        {
        case FILE_CREATION_MESSAGE_ID:
            onFileCreationMessage(&msg);
            break;

        case FILE_TRANSFER_MESSAGE_ID:
            onFileTransferMessage(&msg);
            break;

        case FILE_STATUS_MESSAGE_ID:
            onFileStatusMessage(&msg);
            break;

        case ACK_MESSAGE_ID:
            onAckMessage(&msg);
            break;

        case PING_MESSAGE_ID:
            onPingMessage(&msg);
            break;

        case TRANSFER_ENDED_MESSAGE_ID:
            onTransferEndedMessage(&msg);
            break;

        case AUTH_REQUEST_MESSAGE_ID:
            onAuthRequestMessage(&msg);
            break;

        default:
            Logger::warn("Server client " + to_string(CLIENT_ID) + " received an unknown message type: " + to_string((int)msg.msgType));
            break;
        }
    }
    Logger::debug("Stopped server client consumer thread.");
}

void FileServerClient::onPingMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, PingMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = PingMessage::getClientIdFromMessage(msg->buffer);
    if (CLIENT_ID != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    unsigned int seqNum = PingMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNum);
    Logger::debug("PONG " + to_string(clientId));
}

void FileServerClient::onAckMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, AckMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    unsigned int seqNumber = AckMessage::getSeqNumberFromMessage(msg->buffer);
    Logger::debug("Acked: " + to_string(seqNumber));

    if (getState() == fsc_awaitServerAuthAck)
    {
        setState(fsc_connected);
    }
}

void FileServerClient::onAuthRequestMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, AuthRequestMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = AuthRequestMessage::getClientIdFromMessage(msg->buffer);
    if (CLIENT_ID != clientId)
    {
        Logger::warn("Invalid client id received for AuthRequestMessage! Ignoring message.");
        return;
    }

    unsigned int seqNumber = AuthRequestMessage::getSeqNumberFromMessage(msg->buffer);
    if (!sendMessageQueue->onSequenceNumberAck(seqNumber))
    {
        Logger::error("Invalid sequence number in AuthRequestMessage received: " + to_string(seqNumber) + ". Sequence number was not found!");
        return;
    }

    unsigned int passwordLength = AuthRequestMessage::getPasswordLengthFromMessage(msg->buffer);
    unsigned char *password = AuthRequestMessage::getPasswordFromMessage(msg->buffer, passwordLength);
    string passwordString = string((char *)password, passwordLength);

    // Check if password is correct:
    if (passwordString.compare(user->PASSWORD))
    {
        Logger::warn("Client " + to_string(CLIENT_ID) + " send an invalid password.");
        sendAuthResultMessage(seqNumber, 0b0000);
        setState(fsc_error);
    }
    else
    {
        sendAuthResultMessage(seqNumber, 0b0001);
        setState(fsc_awaitServerAuthAck);
    }
}

void FileServerClient::sendAuthResultMessage(unsigned int seqNumber, unsigned char flags)
{
    AuthResultMessage *msg = new AuthResultMessage(CLIENT_ID, flags, seqNumber);
    udpClient->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
}

void FileServerClient::onFileCreationMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, FileCreationMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = FileCreationMessage::getClientIdFromMessage(msg->buffer);
    if (CLIENT_ID != clientId)
    {
        Logger::warn("Invalid client id received for FileCreationMessage! Ignoring message.");
        return;
    }

    // Send ack:
    unsigned int seqNumber = FileCreationMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNumber);

    // Create file/folder:
    unsigned char fileType = FileCreationMessage::getFileTypeFromMessage(msg->buffer);
    uint64_t fidLengt = FileCreationMessage::getFIDLengthFromMessage(msg->buffer);
    unsigned char *fid = FileCreationMessage::getFIDFromMessage(msg->buffer, fidLengt);
    unsigned char *hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
    string fidString = string((char *)fid, fidLengt);
    switch (fileType)
    {
    case 1:
        user->fS->genFolder(fidString);
        Logger::debug("Folder \"" + fidString + "\" generated.");
        break;

    case 2:
        user->fS->delFolder(fidString);
        break;

    case 4:
        hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
        curFID = fidString;
        curFIDLength = fidLengt;
        user->fS->genFile(curFID, (char *)hash);
        Logger::debug("File \"" + fidString + "\" generated.");
        break;

    case 8:
        user->fS->delFile(fidString);
        break;

    default:
        Logger::error("Unknown fileType received: " + to_string((int)fileType));
        break;
    }
    delete[] fid;
    delete[] hash;
}

void FileServerClient::onFileTransferMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, FileTransferMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = FileTransferMessage::getClientIdFromMessage(msg->buffer);
    if (CLIENT_ID != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    // Send ack:
    unsigned int seqNumber = FileTransferMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNumber);

    // Write file:
    char flags = FileTransferMessage::getFlagsFromMessage(msg->buffer);
    if ((flags & 0b10) == 0b10)
    {
        unsigned int partNumber = FileTransferMessage::getFIDPartNumberFromMessage(msg->buffer);
        uint64_t contLength = FileTransferMessage::getContentLengthFromMessage(msg->buffer);
        unsigned char *content = FileTransferMessage::getContentFromMessage(msg->buffer, contLength);
        int result = user->fS->writeFilePart(curFID, (char *)content, partNumber, contLength, CLIENT_ID);
        Logger::debug("Wrote file part: " + to_string(contLength) + ", length: " + to_string(contLength) + " for file: \"" + curFID + "\" with result: " + to_string(result));
        if ((flags & 0b1000) == 0b1000)
        {
            Logger::info("Last file part received for: " + curFID);
        }
        delete[] content;
    }
    else
    {
        Logger::error("Invalid FileTransferMessage flags received: " + to_string((int)flags));
    }
}

void FileServerClient::onTransferEndedMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, TransferEndedMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = TransferEndedMessage::getClientIdFromMessage(msg->buffer);
    if (CLIENT_ID != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    unsigned char flags = TransferEndedMessage::getFlagsFromMessage(msg->buffer);
    // Transfer finished:
    if ((flags & 0b0001) == 0b0001)
    {
        Logger::info("Client " + to_string(clientId) + " finished upload.");
    }
    // Calceled by user:
    else if ((flags & 0b0010) == 0b0010)
    {
        Logger::info("Client " + to_string(clientId) + " canceled upload.");
        Logger::info("Disconnecting client...");
        user->removeClient(this);
        Logger::info("Client disconnected.");
    }
    // Error:
    else
    {
        Logger::info("Client " + to_string(clientId) + " upload failed.");
        Logger::info("Disconnecting client...");
        user->removeClient(this);
        Logger::info("Client disconnected.");
    }
}

void FileServerClient::onFileStatusMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, FileStatusMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = FileStatusMessage::getClientIdFromMessage(msg->buffer);
    if (CLIENT_ID != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    uint64_t fidLengt = FileStatusMessage::getFIDLengthFromMessage(msg->buffer);
    unsigned char *fid = FileStatusMessage::getFIDFromMessage(msg->buffer, fidLengt);
    string fidString = string((char *)fid, fidLengt);
    unsigned int lastPart = user->fS->getLastPart(fidString);
    unsigned int seqNumber = FileStatusMessage::getSeqNumberFromMessage(msg->buffer);

    unsigned char recFlags = FileStatusMessage::getFlagsFromMessage(msg->buffer);
    unsigned char flags = 0b0010;
    flags |= (recFlags & 0b1000);

    sendFileStatusAnswerMessage(seqNumber, lastPart, flags, curFIDLength, (unsigned char *)curFID.c_str());

    delete[] fid;
}

void FileServerClient::sendServerHelloMessage(unsigned char flags, unsigned long pubKey)
{
    unsigned int seqNumber = getNextSeqNumber();
    ServerHelloMessage *msg = new ServerHelloMessage(PORT_LOCAL, CLIENT_ID, seqNumber, flags, pubKey);
    udpClient->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
}

void FileServerClient::sendAckMessage(unsigned int seqNumber)
{
    AckMessage msg = AckMessage(seqNumber);
    udpClient->send(&msg);
}

void FileServerClient::sendFileStatusAnswerMessage(unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID)
{
    FileStatusMessage msg = FileStatusMessage(CLIENT_ID, seqNumber, lastFIDPartNumber, flags, fIDLength, fID);
    udpClient->send(&msg);
}

void FileServerClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber)
{
    PingMessage *msg = new PingMessage(plLength, seqNumber, CLIENT_ID);

    udpClient->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
    Logger::debug("Ping");
}