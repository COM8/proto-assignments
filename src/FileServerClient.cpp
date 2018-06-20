#include "FileServerClient.h"
#include "FileServerUser.h"

using namespace std;
using namespace net;
using namespace sec;

FileServerClient::FileServerClient(unsigned int clientId, unsigned short portLocal, unsigned short portRemote, char *ipRemote, unsigned int maxPPS, FileServerUser *user) : AbstractClient(clientId),
                                                                                                                                                                            PORT_LOCAL(portLocal),
                                                                                                                                                                            PORT_REMOTE(portRemote),
                                                                                                                                                                            IP_REMOTE(ipRemote)
{
    this->user = user;
    this->maxPPS = maxPPS;

    this->state = fsc_disconnected;
    this->udpServer = new Server2(PORT_LOCAL, cpQueue, enc);
    this->udpClient = new Client2(IP_REMOTE, PORT_REMOTE, maxPPS, enc);
    this->udpClient->init();
    this->lastMessageTime = time(NULL);
    this->curFID = "";
    this->curFIDLength = 0;
    this->lastFIDPartNumber = 0;
    this->state = fsc_disconnected;
    this->toDoHelper = ToDoHelper();
}

FileServerClient::~FileServerClient()
{
    setState(fsc_error);
    delete udpClient;
    delete udpServer;
}

void FileServerClient::onTimerTick(int identifier)
{
    if (identifier == 1)
    {
        switch (getState())
        {
        case fsc_connected:
            if (!toDoHelper.clientToDo)
            {
                toDoHelper.clientToDo = user->getClientToDo(clientId);
            }
            if (!toDoHelper.clientToDo || toDoHelper.clientToDo->isEmpty())
            {
                setState(fsc_ping);
            }
            else
            {
                setState(fsc_sendingChanges);
            }
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
                    Logger::warn("Resending message with type: " + to_string(msg.msg->getType()));
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
    sendServerHelloMessage(PORT_LOCAL, flags, -1, udpClient);
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
    Logger::debug("[FileServerClient " + to_string(clientId) + "]: " + to_string(this->state) + " -> " + to_string(state));
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
        sendServerHelloMessage(PORT_LOCAL, 0b0001, enc->getPubKey(), udpClient);
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
        sendNextClientToDo();
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

void FileServerClient::sendNextClientToDo()
{
    if (!toDoHelper.curToDo)
    {
        toDoHelper.loadNext();
    }

    if (!toDoHelper.curToDo)
    {
        setState(fsc_connected);
        return;
    }

    switch (toDoHelper.curToDo->type)
    {
    case ft_del_folder:
        setState(fsc_awaitingAck);
        sendFolderDeletionMessage(toDoHelper.curToDo->fid, udpClient);
        break;

    case ft_del_file:
        setState(fsc_awaitingAck);
        sendFileDeletionMessage(toDoHelper.curToDo->fid, udpClient);
        break;

    case ft_folder:
        setState(fsc_awaitingAck);
        sendFileCreationMessage(toDoHelper.curToDo->fid, NULL, ft_folder, udpClient);
        break;

    case ft_file:
        setState(fsc_awaitingAck);
        if (toDoHelper.sendCreationMsg)
        {
            unsigned int np = toDoHelper.curToDo->np.getNextPart();
            if (toDoHelper.curToDo->np.isEmpty())
            {
                toDoHelper.clientToDo->removeToDo(toDoHelper.curToDo->fid);
            }
        }
        else
        {
            toDoHelper.sendCreationMsg = true;
            sendFileCreationMessage(toDoHelper.curToDo->fid, toDoHelper.curToDo->hash, ft_file, udpClient);
        }
        break;

    case ft_none:
    default:
        setState(fsc_connected);
        break;
    }
}

void FileServerClient::onMessageReceived(net::ReadMessage *msg)
{
    lastMessageTime = time(NULL);

    switch (msg->msgType)
    {
    case FILE_CREATION_MESSAGE_ID:
        onFileCreationMessage(msg);
        break;

    case FILE_TRANSFER_MESSAGE_ID:
        onFileTransferMessage(msg);
        break;

    case FILE_STATUS_MESSAGE_ID:
        onFileStatusMessage(msg);
        break;

    case ACK_MESSAGE_ID:
        onAckMessage(msg);
        break;

    case PING_MESSAGE_ID:
        onPingMessage(msg);
        break;

    case TRANSFER_ENDED_MESSAGE_ID:
        onTransferEndedMessage(msg);
        break;

    case AUTH_REQUEST_MESSAGE_ID:
        onAuthRequestMessage(msg);
        break;

    default:
        Logger::warn("Server client " + to_string(clientId) + " received an unknown message type: " + to_string((int)msg->msgType));
        break;
    }
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
    if (clientId != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    unsigned int seqNum = PingMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNum, udpClient);
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
    if (!sendMessageQueue->onSequenceNumberAck(seqNumber))
    {
        Logger::error("Unable to ACK sequence number: " + to_string(seqNumber) + ". Sequence number was not found!");
        return;
    }
    else
    {
        Logger::debug("Acked sequence number: " + to_string(seqNumber));
    }

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
    if (clientId != clientId)
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
        Logger::warn("Client " + to_string(clientId) + " send an invalid password.");
        sendAuthResultMessage(seqNumber, 0b0000, udpClient);
        setState(fsc_error);
    }
    else
    {
        Logger::info("Client " + to_string(clientId) + " successfull auth.");
        sendAuthResultMessage(seqNumber, 0b0001, udpClient);
        setState(fsc_awaitServerAuthAck);
    }
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
    if (clientId != clientId)
    {
        Logger::warn("Invalid client id received for FileCreationMessage! Ignoring message.");
        return;
    }

    // Send ack:
    unsigned int seqNumber = FileCreationMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNumber, udpClient);

    // Create file/folder:
    unsigned char fileType = FileCreationMessage::getFileTypeFromMessage(msg->buffer);
    uint64_t fidLengt = FileCreationMessage::getFIDLengthFromMessage(msg->buffer);
    unsigned char *fid = FileCreationMessage::getFIDFromMessage(msg->buffer, fidLengt);
    unsigned char *hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
    string fidString = string((char *)fid, fidLengt);
    switch (fileType)
    {
    case 1:
        user->fS->genFolder(fidString, clientId);
        Logger::debug("Folder \"" + fidString + "\" generated.");
        break;

    case 2:
        user->fS->delFolder(fidString, clientId);
        break;

    case 4:
        hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
        curFID = fidString;
        curFIDLength = fidLengt;
        user->fS->genFile(curFID, (char *)hash);
        Logger::debug("File \"" + fidString + "\" generated.");
        break;

    case 8:
        user->fS->delFile(fidString, clientId);
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
    if (clientId != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    // Send ack:
    unsigned int seqNumber = FileTransferMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNumber, udpClient);

    // Write file:
    char flags = FileTransferMessage::getFlagsFromMessage(msg->buffer);
    if ((flags & 0b10) == 0b10)
    {
        unsigned int partNumber = FileTransferMessage::getFIDPartNumberFromMessage(msg->buffer);
        uint64_t contLength = FileTransferMessage::getContentLengthFromMessage(msg->buffer);
        unsigned char *content = FileTransferMessage::getContentFromMessage(msg->buffer, contLength);
        int result = user->fS->writeFilePart(curFID, (char *)content, partNumber, contLength, clientId);
        Logger::debug("Wrote file part: " + to_string(partNumber) + ", length: " + to_string(contLength) + " for file: \"" + curFID + "\" with result: " + to_string(result));
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
    if (clientId != clientId)
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
    if (clientId != clientId)
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

    sendFileStatusAnswerMessage(seqNumber, lastPart, flags, curFIDLength, (unsigned char *)curFID.c_str(), udpClient);

    delete[] fid;
}