#include "AbstractClient.h"

using namespace std;
using namespace net;
using namespace sec;

AbstractClient::AbstractClient(unsigned int clientId)
{
    this->clientId = clientId;
    this->seqNumberMutex = new mutex();
    this->seqNumber = 0;
    this->shouldConsumerRun = false;
    this->enc = NULL;
    this->tTimer = new Timer(true, TIMER_TICK_INTERVALL_MS, this, 1);
    this->cpQueue = new Queue<ReadMessage>();
    this->sendMessageQueue = new SendMessageQueue();
}
AbstractClient::~AbstractClient()
{
    stopConsumerThread();
    tTimer->stop();

    delete seqNumberMutex;
    delete enc;
    delete cpQueue;
    delete tTimer;
    delete sendMessageQueue;
}

unsigned int AbstractClient::getNextSeqNumber()
{
    unique_lock<mutex> mlock(*seqNumberMutex);
    unsigned int i = seqNumber++;
    mlock.unlock();
    return i;
}

void AbstractClient::sendFileTransferMessage(unsigned char flags, unsigned int fidPartNumber, uint64_t contentLength, unsigned char *content, string fid, Client2 *client)
{
    unsigned int i = getNextSeqNumber();
    FileTransferMessage *msg = new FileTransferMessage(clientId, i, flags, fidPartNumber, (uint64_t)contentLength, (unsigned char *)content);

    client->send(msg);
    sendMessageQueue->pushSendMessage(i, msg);
    Logger::debug("Send file part " + to_string(fidPartNumber) + ", length: " + to_string(contentLength) + " for file: " + fid);
}

void AbstractClient::sendTransferEndedMessage(unsigned char flags, Client2 *client)
{
    TransferEndedMessage msg = TransferEndedMessage(clientId, flags);
    client->send(&msg);
}

void AbstractClient::sendFolderCreationMessage(shared_ptr<Folder> f, Client2 *client)
{
    const char *c = f->path.c_str();
    uint64_t l = f->path.length();
    int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 1, NULL, l, (unsigned char *)c);

    client->send(msg);
    sendMessageQueue->pushSendMessage(i, msg);
    Logger::info("Send folder creation for: \"" + f->path + "\"");
}

void AbstractClient::sendFolderDeletionMessage(string folderPath, Client2 *client)
{
    const char *c = folderPath.c_str();
    uint64_t l = folderPath.length();
    int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 2, NULL, l, (unsigned char *)c);

    client->send(msg);
    sendMessageQueue->pushSendMessage(i, msg);
    Logger::info("Send folder deletion for: \"" + folderPath + "\"");
}

void AbstractClient::sendFileDeletionMessage(string filePath, Client2 *client)
{
    const char *c = filePath.c_str();
    uint64_t l = filePath.length();
    int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 8, NULL, l, (unsigned char *)c);

    client->send(msg);
    sendMessageQueue->pushSendMessage(i, msg);
    Logger::info("Send file deletion for: \"" + filePath + "\"");
}

void AbstractClient::sendFileCreationMessage(string fid, std::shared_ptr<File> f, Client2 *client)
{
    const char *c = fid.c_str();
    uint64_t l = fid.length();
    unsigned int i = getNextSeqNumber();
    FileCreationMessage *msg = new FileCreationMessage(clientId, i, 4, (unsigned char *)f->hash.get()->data(), l, (unsigned char *)c);

    client->send(msg);
    sendMessageQueue->pushSendMessage(i, msg);
    Logger::info("Send file creation: " + fid);
}

void AbstractClient::sendFileStatusMessage(string fid, struct std::shared_ptr<File> f, Client2 *client)
{
    const char *c = fid.c_str();
    uint64_t l = fid.length();
    unsigned int i = getNextSeqNumber();
    FileStatusMessage *msg = new FileStatusMessage(clientId, i, 9, l, (unsigned char *)c);

    client->send(msg);
    sendMessageQueue->pushSendMessage(i, msg);
    Logger::info("Requested file status for: " + fid);
}

void AbstractClient::sendClientHelloMessage(unsigned short listenPort, unsigned char flags, string username, Client2 *client)
{
    unsigned char *c = (unsigned char *)username.c_str();
    unsigned int l = username.length();
    ClientHelloMessage msg = ClientHelloMessage(listenPort, clientId, flags, enc->getPrime(), enc->getPrimitiveRoot(), enc->getPubKey(), c, l);
    client->send(&msg);
}

void AbstractClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber, Client2 *client)
{
    PingMessage *msg = new PingMessage(plLength, seqNumber, clientId);

    client->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
    Logger::debug("Ping");
}

void AbstractClient::sendAuthRequestMessage(unsigned int seqNumber, string password, Client2 *client)
{
    const char *c = password.c_str();
    unsigned int l = password.length();

    AuthRequestMessage *msg = new AuthRequestMessage(clientId, l, (unsigned char *)c, seqNumber);

    client->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
    Logger::debug("Send auth request with sequence number: " + to_string(seqNumber));
}

void AbstractClient::sendAuthResultMessage(unsigned int seqNumber, unsigned char flags, Client2 *client)
{
    AuthResultMessage *msg = new AuthResultMessage(clientId, flags, seqNumber);
    client->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
}

void AbstractClient::sendServerHelloMessage(unsigned short portLocal, unsigned char flags, unsigned long pubKey, Client2 *client)
{
    unsigned int seqNumber = getNextSeqNumber();
    ServerHelloMessage *msg = new ServerHelloMessage(portLocal, clientId, seqNumber, flags, pubKey);
    client->send(msg);
    sendMessageQueue->pushSendMessage(seqNumber, msg);
}

void AbstractClient::sendFileStatusAnswerMessage(unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID, Client2 *client)
{
    FileStatusMessage msg = FileStatusMessage(clientId, seqNumber, lastFIDPartNumber, flags, fIDLength, fID);
    client->send(&msg);
}

void AbstractClient::sendAckMessage(unsigned int seqNumber, net::Client2 *client)
{
    AckMessage msg = AckMessage(seqNumber);
    client->send(&msg);
}

void AbstractClient::startConsumerThread()
{
    shouldConsumerRun = true;
    consumerThread = thread(&AbstractClient::consumerTask, this);
}

void AbstractClient::stopConsumerThread()
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

void AbstractClient::consumerTask()
{
    Logger::debug("Started consumer thread.");
    while (shouldConsumerRun)
    {
        ReadMessage msg = cpQueue->pop();
        if (!shouldConsumerRun)
        {
            break;
        }

        onMessageReceived(&msg);
    }
    Logger::debug("Stopend consumer thread.");
}