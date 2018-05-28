#include "FileClient2.h"

using namespace net;
using namespace std;

FileClient2::FileClient2(string serverAddress, unsigned short serverPort, string userName, string clientPassword, FilesystemClient *fS)
{
    this->serverAddress = serverAddress;
    this->serverPort = serverPort;
    this->userName = userName;
    this->clientPassword = clientPassword;
    this->fS = fS;

    srand(time(NULL));

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
    this->transferFinished = false;
    this->tTimer = new Timer(true, TIMER_TICK_INTERVALL_MS, this, 1);
    this->curWorkingSet = fS->getWorkingSet();
    this->clientId = getRandomClientId();
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
    delete curWorkingSet;
    delete tTimer;
}

unsigned int FileClient2::getRandomClientId()
{
    return rand();
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
        if (!transferFinished)
        {
            if (reconnect)
            {
            }
            else
            {
                startSendingFS();
            }
        }
        tTimer->start();
        break;

    case ping:
        tTimer->reset();
        sendPingMessage(0, getNextSeqNumber(), uploadClient);
        break;

    case reqestedFileStatus:
        tTimer->reset();
        break;

    case sendingFS:
        tTimer->stop();
        sendNextFilePart();
        break;

    case awaitingAck:
        tTimer->start();
        break;

    case resending:
        tTimer->stop();
        break;
    }
}

void FileClient2::sendNextFilePart()
{
    std::list<std::shared_ptr<Folder>> *folders = curWorkingSet->getFolders();
    std::unordered_map<std::string, std::shared_ptr<File>> *files = curWorkingSet->getFiles();
    std::list<std::string> *delFolders = curWorkingSet->getDelFolders();
    std::list<std::string> *delFiles = curWorkingSet->getDelFiles();

    // Continue file transfer:
    int curFilePartNr = curWorkingSet->getCurFilePartNr();
    if (curFilePartNr >= 0)
    {
        string fid = curWorkingSet->getCurFID();
        shared_ptr<File> curFile = curWorkingSet->getCurFileFile();
        setState(awaitingAck);
        bool lastPartSend = sendFilePartMessage(fid, curFile, curFilePartNr, uploadClient);
        curWorkingSet->unlockCurFile();

        curWorkingSet->setCurFilePartNr(++curFilePartNr);

        if (lastPartSend)
        {
            files->erase(curWorkingSet->getCurFID());
            curWorkingSet->setCurFilePartNr(-1);
        }
    }
    // Delete file:
    else if (!delFiles->empty())
    {
        string filePath = delFiles->front();
        delFiles->pop_front();
        setState(awaitingAck);
        sendFileDeletionMessage(filePath, uploadClient);
    }
    // Delete folder:
    else if (!delFolders->empty())
    {
        string folderPath = delFolders->front();
        delFolders->pop_front();
        setState(awaitingAck);
        sendFileDeletionMessage(folderPath, uploadClient);
    }
    // Trasfer folder:
    else if (!folders->empty())
    {
        shared_ptr<Folder> f = folders->front();
        folders->pop_front();
        setState(awaitingAck);
        sendFolderCreationMessage(f, uploadClient);
    }
    // Transfer file:
    else if (!files->empty())
    {
        curWorkingSet->setCurFile(files->begin()->first, files->begin()->second);
        curWorkingSet->setCurFilePartNr(0);

        string fid = curWorkingSet->getCurFID();
        shared_ptr<File> curFile = curWorkingSet->getCurFileFile();
        setState(reqestedFileStatus);
        sendFileStatusMessage(fid, curFile, uploadClient);
        curWorkingSet->unlockCurFile();
    }
    else
    {
        curWorkingSet->unlockFiles();
        curWorkingSet->unlockFolders();
        curWorkingSet->unlockDelFiles();
        curWorkingSet->unlockDelFolders();

        if (curWorkingSet->isEmpty())
        {
            delete curWorkingSet;
            curWorkingSet = fS->getWorkingSet();
            if (curWorkingSet->isEmpty())
            {
                sendTransferEndedMessage(0b0001, uploadClient);
                transferFinished = true;
                Logger::info("Transfer finished!");
                setState(connected);
                return;
            }
        }
        sendNextFilePart();
        return;
    }

    curWorkingSet->unlockFiles();
    curWorkingSet->unlockFolders();
    curWorkingSet->unlockDelFiles();
    curWorkingSet->unlockDelFolders();
}

void FileClient2::sendTransferEndedMessage(unsigned char flags, Client *client)
{
    TransferEndedMessage *msg = new TransferEndedMessage(clientId, flags);
    client->send(msg);
}

void FileClient2::sendFolderCreationMessage(shared_ptr<Folder> f, Client *client)
{
    const char *c = f->path.c_str();
    uint64_t l = f->path.length();
    int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 1, NULL, l, (unsigned char *)c);
    sendMessageQueue->pushSendMessage(i, msg);

    client->send(msg);
    Logger::info("Send folder creation for: \"" + f->path + "\"");
}

void FileClient2::sendFolderDeletionMessage(string folderPath, Client *client)
{
    const char *c = folderPath.c_str();
    uint64_t l = folderPath.length();
    int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 2, NULL, l, (unsigned char *)c);
    sendMessageQueue->pushSendMessage(i, msg);

    client->send(msg);
    Logger::info("Send folder deletion for: \"" + folderPath + "\"");
}

void FileClient2::sendFileDeletionMessage(string filePath, Client *client)
{
    const char *c = filePath.c_str();
    uint64_t l = filePath.length();
    int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 8, NULL, l, (unsigned char *)c);
    sendMessageQueue->pushSendMessage(i, msg);

    client->send(msg);
    Logger::info("Send file deletion for: \"" + filePath + "\"");
}

void FileClient2::sendFileCreationMessage(string fid, std::shared_ptr<File> f, Client *client)
{
    const char *c = fid.c_str();
    uint64_t l = fid.length();
    unsigned int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 4, (unsigned char *)f->hash, l, (unsigned char *)c);
    sendMessageQueue->pushSendMessage(i, msg);

    client->send(msg);
    Logger::info("Send file creation: " + fid);
}

void FileClient2::sendFileStatusMessage(string fid, struct std::shared_ptr<File> f, Client *client)
{
    const char *c = fid.c_str();
    uint64_t l = fid.length();
    unsigned int i = getNextSeqNumber();
    FileStatusMessage *msg = new FileStatusMessage(clientId, i, 9, l, (unsigned char *)c);
    sendMessageQueue->pushSendMessage(i, msg);

    client->send(msg);
    Logger::info("Requested file status for: " + fid);
}

bool FileClient2::sendFilePartMessage(string fid, shared_ptr<File> f, unsigned int nextPartNr, Client *client)
{
    char chunk[Filesystem::partLength];
    bool isLastPart = false;
    int readCount = fS->readFile(fid, chunk, nextPartNr, &isLastPart);

    // File part:
    char flags = 2;

    // First file part:
    if (nextPartNr == 0)
    {
        flags |= 1;
    }
    // Last file part:
    else if (isLastPart)
    {
        flags |= 8;
    }

    unsigned int i = getNextSeqNumber();
    FileTransferMessage *msg = new FileTransferMessage(clientId, i, flags, nextPartNr, (unsigned char *)f->hash, (uint64_t)readCount, (unsigned char *)chunk);
    sendMessageQueue->pushSendMessage(i, msg);

    client->send(msg);
    Logger::info("Send file part " + to_string(nextPartNr) + ", length: " + to_string(readCount) + " for file: " + fid);
    return isLastPart;
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
        printToDo();
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

void FileClient2::onFileStatusMessage(net::ReadMessage *msg)
{
    if (getState() != reqestedFileStatus)
    {
        Logger::error("FileClient2::onFileStatusMessage invalid state: " + to_string(state));
        return;
    }

    // Check if checksum is valid:
    if (!AbstractMessage::isChecksumValid(msg, FileStatusMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check sequence number:
    unsigned int seqNumber = FileStatusMessage::getSeqNumberFromMessage(msg->buffer);
    if (!sendMessageQueue->onSequenceNumberAck(seqNumber))
    {
        Logger::error("Invalid sequence number in FileStatusMessage received: " + to_string(seqNumber) + ". Sequence number was not found!");
        return;
    }
    else
    {
        Logger::debug("Acked sequence number in FileStatusMessage: " + to_string(seqNumber));
    }

    // Check flags:
    unsigned char flags = FileStatusMessage::getFlagsFromMessage(msg->buffer);
    if ((flags & 0b0010) != 0b0010)
    {
        Logger::warn("Received invalid flags for FileStatusMessage: " + to_string(flags) + " " + to_string((flags & 0b0010)));
        return;
    }

    unsigned int lastFIDPartNumber = FileStatusMessage::getLastFIDPartNumberFromMessage(msg->buffer);

    if (lastFIDPartNumber <= 0)
    {
        string fid = curWorkingSet->getCurFID();
        shared_ptr<File> curFile = curWorkingSet->getCurFileFile();
        setState(awaitingAck);
        sendFileCreationMessage(fid, curFile, uploadClient);
        curWorkingSet->unlockCurFile();
    }
    else
    {
        curWorkingSet->setCurFilePartNr(lastFIDPartNumber);
        setState(sendingFS);
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
            onFileStatusMessage(&msg);
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
            if ((flags & 0b0100) == 0b0100)
            {
                unsigned int recClientId = ServerHelloMessage::getClientIdFromMessage(msg->buffer);
                if (recClientId == clientId)
                {
                    Logger::warn("Client ID " + to_string(clientId) + " already taken. Changing ID");
                    clientId = getRandomClientId();
                    Logger::info("Changed client ID to: " + to_string(clientId));
                }
                else
                {
                    Logger::debug("Dropped ServerHelloMessage with invalid client ID.");
                }
            }
            else
            {
                Logger::error("Client not accepted: " + to_string(flags));
                setState(disconnected);
            }
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

    case reqestedFileStatus:
    case awaitingAck:
    {
        list<struct SendMessage> *msgs = new list<struct SendMessage>();
        sendMessageQueue->popNotAckedMessages(MAX_ACK_TIME_IN_S, msgs);

        for (struct SendMessage msg : *msgs)
        {
            if (msg.sendCount > MAX_MESSAGE_SEND_TRIES)
            {
                Logger::error("Failed to send messge " + to_string(msg.sendCount) + " times - reconnecting!");
                disconnect();
                reconnect = true;
                connect();
            }
            else
            {
                // Resend message:
                uploadClient->send(msg.msg);
                msg.sendCount++;
                msg.sendTime = time(NULL);
                sendMessageQueue->push(msg);
                Logger::info("Resending message.");
            }
        }
        delete msgs;
    }
    break;

    case ping:
        sendMessageQueue->clear();
        msgTimeoutCount++;
        Logger::warn("Ping timeout, msgTimeoutCount: " + to_string(msgTimeoutCount));
        if (msgTimeoutCount < 3)
        {
            Logger::warn("Resending ping");
            sendPingMessage(0, getNextSeqNumber(), uploadClient);
            setState(connected);
        }
        else
        {
            Logger::error("Server timed out - reconnecting...");
            disconnect();
            reconnect = true;
            connect();
        }
        break;

    default:
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
        return;
    }
    else
    {
        Logger::debug("Acked sequence number: " + to_string(seqNumber));
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

    default:
        break;
    }
}

void FileClient2::printToDo()
{
    cout << "ToDo list:" << endl;
    if (!curWorkingSet)
    {
        cout << "Nothing to do!" << endl;
    }
    else
    {
        cout << "Files: " << curWorkingSet->getFiles()->size() << endl;
        cout << "Folders: " << curWorkingSet->getFolders()->size() << endl;
        cout << "Delete files: " << curWorkingSet->getDelFiles()->size() << endl;
        cout << "Delete folders: " << curWorkingSet->getDelFolders()->size() << endl;
        cout << "Current file: ";
        if (curWorkingSet->getCurFilePartNr() >= 0)
        {
            cout << "FID: " << curWorkingSet->getCurFID() << ", Part: " << curWorkingSet->getCurFilePartNr() << endl;
        }
        else
        {
            cout << "None" << endl;
        }

        // Unlock workingset:
        curWorkingSet->unlockCurFile();
        curWorkingSet->unlockDelFiles();
        curWorkingSet->unlockDelFolders();
        curWorkingSet->unlockFiles();
        curWorkingSet->unlockFolders();
    }
}