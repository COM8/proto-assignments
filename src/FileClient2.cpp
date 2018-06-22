#include "FileClient2.h"

using namespace net;
using namespace std;
using namespace sec;

FileClient2::FileClient2(string serverAddress, unsigned short serverPort, string username, string clientPassword, FilesystemClient *fS) : AbstractClient(123456789)
{
    this->serverAddress = serverAddress;
    this->serverPort = serverPort;
    this->username = username;
    this->clientPassword = clientPassword;
    this->fS = fS;

    srand(time(NULL));

    this->state = disconnected;
    this->stateMutex = new mutex();
    this->shouldHelloRun = false;
    this->client = NULL;
    this->uploadClient = NULL;
    this->server = NULL;
    this->reconnect = false;
    this->msgTimeoutCount = 0;
    this->uploadPort = 0;
    this->curWorkingSet = NULL;
    this->clientId = getRandomClientId();
    this->gettingWorkingSet = false;
    this->joinedWorkingSetThread = true;
    this->curFID = "";
}

FileClient2::~FileClient2()
{
    disconnect();
    stopHelloThread();
    joinGetWorkingSet();

    delete stateMutex;
    delete client;
    delete uploadClient;
    delete server;
    delete curWorkingSet;
}

unsigned int FileClient2::getRandomClientId()
{
    return rand();
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
        if (server)
        {
            server->stop();
            delete server;
            server = NULL;
        }
        if (client)
        {
            delete client;
            client = NULL;
        }
        break;

    case sendClientHello:
        // Setup diffi:
        if (enc)
        {
            delete enc;
            enc = NULL;
        }
        enc = new DiffieHellman();
        enc->clientStartConnection();

        msgTimeoutCount = 0;
        startConsumerThread();
        startHelloThread();
        break;

    case clientAuth:
        tTimer->start();
        break;

    case connected:
        tTimer->start();
        if (!gettingWorkingSet && joinedWorkingSetThread && !curWorkingSet->isEmpty())
        {
            startSendingFS();
        }
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
    int curFilePartNr = curWorkingSet->getCurNextPart();
    curWorkingSet->unlockCurFile();
    if (curFilePartNr >= 0)
    {
        string fid = curWorkingSet->getCurFID();
        shared_ptr<File> curFile = curWorkingSet->getCurFileFile();
        setState(awaitingAck);
        bool lastPartSend = sendFilePartMessage(fid, curFile, curFilePartNr, uploadClient);
        curWorkingSet->unlockCurFile();

        curWorkingSet->getCurFileFile()->np->acknowledgePart(curFilePartNr);
        curWorkingSet->unlockCurFile();
        if (lastPartSend)
        {
            files->erase(curWorkingSet->getCurFID());
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
            startGetWorkingSet();
            joinGetWorkingSet();
            if (curWorkingSet->isEmpty())
            {
                sendTransferEndedMessage(0b0001, uploadClient);
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

bool FileClient2::sendFilePartMessage(string fid, shared_ptr<File> f, unsigned int nextPartNr, Client2 *client)
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

    sendFileTransferMessage(flags, nextPartNr, readCount, (unsigned char *)chunk, fid, uploadClient);

    if (isLastPart)
    {
        Logger::info("Finished sending: " + fid);
    }
    return isLastPart;
}

void FileClient2::connect()
{
    FileClient2State state = getState();
    if (state == disconnected)
    {
        startGetWorkingSet();
        joinGetWorkingSet();
        listenPort = 2000 + rand() % 63000; // Pick random lisen port
        client = new Client2(serverAddress, serverPort, UNLIMITED_PPS, enc);
        client->init();
        server = new Server2(listenPort, cpQueue, enc);
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
    helloThread = thread(&FileClient2::helloTask, this, listenPort, reconnect, client);
}

void FileClient2::startGetWorkingSet()
{
    if (!gettingWorkingSet)
    {
        gettingWorkingSet = true;
        workingSetThread = thread(&FileClient2::getWorkingSet, this);
    }
    else
    {
        Logger::warn("Faild to start get workingset thread. Join first!");
    }
}

void FileClient2::joinGetWorkingSet()
{
    if (workingSetThread.joinable() && workingSetThread.get_id() != this_thread::get_id())
    {
        workingSetThread.join();
    }
    joinedWorkingSetThread = true;
}

void FileClient2::getWorkingSet()
{
    if (curWorkingSet)
    {
        delete curWorkingSet;
        curWorkingSet = NULL;
    }
    lastGetWorkingSet = time(NULL);
    Logger::info("Started indexing files.");
    curWorkingSet = fS->getWorkingSet();
    int div = (int)difftime(time(NULL), lastGetWorkingSet);
    Logger::info("Finished indexing files in " + to_string(div) + " seconds.");
    gettingWorkingSet = false;
    joinedWorkingSetThread = false;
}

void FileClient2::stopHelloThread()
{
    shouldHelloRun = false;
    if (helloThread.joinable() && helloThread.get_id() != this_thread::get_id())
    {
        helloThread.join();
    }
}

void FileClient2::helloTask(unsigned short listenPort, bool reconnecting, Client2 *client)
{
    Logger::debug("Started hello thread.");
    while (getState() == sendClientHello && shouldHelloRun)
    {
        unsigned char flags = 0b0001;
        if (reconnecting)
        {
            flags |= 0b0010;
        }
        sendClientHelloMessage(listenPort, flags, username, client);
        sleep(1); // Sleep for 1 second
    }
    Logger::debug("Stopped hello thread.");
}

void FileClient2::onAuthResultMessage(net::ReadMessage *msg)
{
    // Check if checksum is valid:
    if (!AbstractMessage::isChecksumValid(msg, AuthResultMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check sequence number:
    unsigned int seqNumber = AuthResultMessage::getSeqNumberFromMessage(msg->buffer);
    if (!sendMessageQueue->onSequenceNumberAck(seqNumber))
    {
        Logger::error("Invalid sequence number in AuthResultMessage received: " + to_string(seqNumber) + ". Sequence number was not found!");
        return;
    }
    else
    {
        sendAckMessage(seqNumber, uploadClient);
        Logger::debug("Acked sequence number in AuthResultMessage: " + to_string(seqNumber));
    }

    if (getState() != clientAuth)
    {
        return;
    }

    unsigned char flags = AuthResultMessage::getFlagsFromMessage(msg->buffer);
    if ((flags & 0b0001) == 0b0001)
    {
        Logger::info("Authentification successfull!");
        Logger::info("Connected to: " + serverAddress + " on port: " + to_string(uploadPort));
        setState(connected);
    }
    else
    {
        Logger::error("Authentification failed with: " + to_string(flags));
        disconnect();
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
        sendFileCreationMessage(fid, (unsigned char *)curFile->hash.get()->data(), uploadClient);
        curWorkingSet->unlockCurFile();
    }
    else
    {
        curWorkingSet->getCurFileFile()->np->acknowledgePart(lastFIDPartNumber);
        curWorkingSet->unlockCurFile();
        setState(sendingFS);
    }
}

void FileClient2::onMessageReceived(ReadMessage *msg)
{
    switch (msg->msgType)
    {
    case SERVER_HELLO_MESSAGE_ID:
        onServerHelloMessage(msg);
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

    case AUTH_RESULT_MESSAGE_ID:
        onAuthResultMessage(msg);
        break;

        /*case 7:
            onTransferEndedMessage(&msg);
            break;*/

    default:
        Logger::warn("Unknown message type received: " + to_string(msg->msgType));
        break;
    }
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

        // Check client ID:
        unsigned int recClientId = ServerHelloMessage::getClientIdFromMessage(msg->buffer);
        if (recClientId != clientId)
        {
            Logger::debug("Dropped ServerHelloMessage with invalid client ID.");
            return;
        }

        unsigned char flags = ServerHelloMessage::getFlagsFromMessage(msg->buffer);
        if ((flags & 0b1000) == 0b1000)
        {
            Logger::error("Client2 not accepted with unknown reason: " + to_string(flags));
            disconnect();
            return;
        }
        else if ((flags & 0b0100) == 0b0100)
        {
            Logger::warn("Client2 ID " + to_string(clientId) + " already taken. Changing ID");
            clientId = getRandomClientId();
            Logger::info("Changed client ID to: " + to_string(clientId));
            return;
        }
        else if ((flags & 0b0010) == 0b0010)
        {
            Logger::error("Client2 not accepted. Server has too many clients. Try again later.");
            disconnect();
            return;
        }

        uploadPort = ServerHelloMessage::getPortFromMessage(msg->buffer);
        uploadClient = new Client2(serverAddress, uploadPort, UNLIMITED_PPS, enc);
        uploadClient->init();

        unsigned int seqNumber = ServerHelloMessage::getSeqNumberFromMessage(msg->buffer);
        sendAuthRequestMessage(seqNumber, clientPassword, uploadClient);

        setState(clientAuth);
    }
    else
    {
        Logger::error("FileClient2::onServerHelloMessage invalid state: " + to_string(state));
    }
}

void FileClient2::disconnect()
{
    Logger::info("Disconnecting...");
    setState(disconnected);
}

void FileClient2::onTimerTick(int identifier)
{
    FileClient2State state = getState();

    switch (state)
    {
    case connected:
        if (!gettingWorkingSet)
        {
            if (!joinedWorkingSetThread)
            {
                joinGetWorkingSet();
            }
            if (difftime(time(NULL), lastGetWorkingSet) > MAX_WORKING_SET_AGE_IN_SEC)
            {
                startGetWorkingSet();
            }
        }
        setState(ping);
        break;

    case reqestedFileStatus:
    case awaitingAck:
    case clientAuth:
    {
        list<struct SendMessage> *msgs = new list<struct SendMessage>();
        sendMessageQueue->popNotAckedMessages(MAX_ACK_TIME_IN_S, msgs);

        for (struct SendMessage msg : *msgs)
        {
            if (msg.sendCount > MAX_MESSAGE_TIMEOUT_COUNT)
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
                Logger::warn("Resending message with type: " + to_string(msg.msg->getType()));
            }
        }
        delete msgs;
    }
    break;

    case ping:
        sendMessageQueue->clear();
        msgTimeoutCount++;
        Logger::warn("Ping timeout, msgTimeoutCount: " + to_string(msgTimeoutCount));
        if (msgTimeoutCount < MAX_PING_MESSAGE_TIMEOUT_COUNT)
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
        setState(connected);
        break;

    default:
        break;
    }
}

void FileClient2::onPingMessage(net::ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, PingMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    // Check if client ID is valid:
    unsigned int clientId = PingMessage::getClientIdFromMessage(msg->buffer);
    if (this->clientId != clientId)
    {
        Logger::warn("Invalid client id received!");
        return;
    }

    unsigned int seqNum = PingMessage::getSeqNumberFromMessage(msg->buffer);
    sendAckMessage(seqNum, uploadClient);
    Logger::debug("PONG " + to_string(clientId));
}

void FileClient2::onFileCreationMessage(ReadMessage *msg)
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
    sendAckMessage(seqNumber, uploadClient);

    // Create file/folder:
    unsigned char fileType = FileCreationMessage::getFileTypeFromMessage(msg->buffer);
    uint64_t fidLengt = FileCreationMessage::getFIDLengthFromMessage(msg->buffer);
    unsigned char *fid = FileCreationMessage::getFIDFromMessage(msg->buffer, fidLengt);
    unsigned char *hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
    string fidString = string((char *)fid, fidLengt);
    switch (fileType)
    {
    case ft_folder:
        fS->genFolder(fidString);
        Logger::debug("Folder \"" + fidString + "\" generated.");
        break;

    case ft_del_folder:
        fS->delFolder(fidString);
        break;

    case ft_file:
        hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
        curFID = fidString;
        fS->genFile(curFID, (char *)hash);
        Logger::debug("File \"" + fidString + "\" generated.");
        break;

    case ft_del_file:
        fS->delFile(fidString);
        break;

    default:
        Logger::error("Unknown fileType received: " + to_string((int)fileType));
        break;
    }
    delete[] fid;
    delete[] hash;
}

void FileClient2::onFileTransferMessage(ReadMessage *msg)
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
    sendAckMessage(seqNumber, uploadClient);

    // Write file:
    char flags = FileTransferMessage::getFlagsFromMessage(msg->buffer);
    if ((flags & 0b10) == 0b10)
    {
        unsigned int partNumber = FileTransferMessage::getFIDPartNumberFromMessage(msg->buffer);
        uint64_t contLength = FileTransferMessage::getContentLengthFromMessage(msg->buffer);
        unsigned char *content = FileTransferMessage::getContentFromMessage(msg->buffer, contLength);
        int result = fS->writeFilePart(curFID, (char *)content, partNumber, contLength);
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
        curWorkingSet->unlockCurFile();
        if (curWorkingSet->curFileExists())
        {
            cout << "FID: " << curWorkingSet->getCurFID() << ", Part: " << curWorkingSet->getCurNextPart() << endl;
        }
        else
        {
            cout << "None" << endl;
        }

        // Unlock workingset:
        curWorkingSet->unlockDelFiles();
        curWorkingSet->unlockDelFolders();
        curWorkingSet->unlockFiles();
        curWorkingSet->unlockFolders();
    }
}