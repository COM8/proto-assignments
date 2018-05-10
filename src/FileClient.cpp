#include "FileClient.h"

using namespace net;
using namespace std;

FileClient::FileClient(std::string *serverAddress, unsigned short serverPort, FilesystemClient *fS)
{
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->fS = fS;
	this->curWorkingSet = NULL;
	this->state = disconnected;
	this->cpQueue = new Queue<ReadMessage>();
	this->sendMessageQueue = new SendMessageQueue();
	this->shouldConsumerRun = false;
	this->shouldTransferRun = false;
	this->shouldHelloRun = false;
	this->consumerThread = NULL;
	this->seqNumber = 0;
	srand (time(NULL));
	this->clientId = rand();
	this->listeningPort = 2000 + rand() % 63000;
	this->server = NULL;
	this->client = NULL;
	this->uploadClient = NULL;
	this->seqNumberMutex = new mutex();
}

void FileClient::startSendingFS()
{
	shouldTransferRun = true;

	client = new Client(*serverAddress, serverPort);

	server = new Server(listeningPort, cpQueue);
	server->start();
	startConsumerThread();
	startHelloThread();
}

void FileClient::startHelloThread()
{
	shouldHelloRun = true;
	helloThread = new thread(&FileClient::helloTask, this, listeningPort);
}
void FileClient::stopHelloThread()
{
	shouldHelloRun = false;
	if (helloThread && helloThread->joinable())
	{
		helloThread->join();
	}
}

void FileClient::helloTask(unsigned short listenPort)
{
	state = sendHandshake;
	while (state < connected && shouldHelloRun)
	{
		sendClientHelloMessage(listenPort, client);
		sleep(1); // Sleep for 1 second
	}
}

void FileClient::startConsumerThread()
{
	shouldConsumerRun = true;
	consumerThread = new thread(&FileClient::consumerTask, this);
}

void FileClient::stopConsumerThread()
{
	shouldConsumerRun = false;
	if (consumerThread && consumerThread->joinable())
	{
		consumerThread->join();
	}
}

void FileClient::onServerHelloMessage(ReadMessage *msg)
{
	// Check if the checksum of the received package is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, ServerHelloMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	if (state == sendHandshake)
	{
		state = connected;
		// Check if client ID was still available:
		if(ServerHelloMessage::getFlagsFromMessage(msg->buffer) & 1 != 1) {
			cout << "Client ID already taken! Trying a new one." << endl;
			stopHelloThread();
			clientId = rand();
			state = disconnected;
			startHelloThread();
			return;
		}

		cout << "Connected to server!" << endl;
		clientId = ServerHelloMessage::getClientIdFromMessage(msg->buffer);
		unsigned short uploadPort = ServerHelloMessage::getPortFromMessage(msg->buffer);
		uploadClient = new Client(*serverAddress, uploadPort);

		state = sendingFS;
		sendNextFilePart();
	}
}

void FileClient::consumerTask()
{
	while (shouldConsumerRun)
	{
		ReadMessage msg = cpQueue->pop();
		// cout << msg.msgType << endl;
		switch (msg.msgType)
		{
		case 2:
			onServerHelloMessage(&msg);
			break;

		case 5:
			onAckMessage(&msg);
			break;

		case 7:
			onTransferEndedMessage(&msg);
			break;

		default:
			cerr << "Unknown message type received: " << msg.msgType << endl;
			break;
		}
	}
}

void FileClient::onAckMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, AckMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int seqNumber = AckMessage::getSeqNumberFromMessage(msg->buffer);
	if (sendMessageQueue->onSequenceNumberAck(seqNumber))
	{
		// cout << "Acked: " << seqNumber << endl;
	}
	else
	{
		cerr << "Sequence number not found: " << seqNumber << endl;
	}

	if (state != awaitingAck)
	{
		cerr << "Received ACK message but state is not awaitingAck!" << endl;
		return;
	}

	// Update state:
	state = sendingFS;

	sendNextFilePart();
	// sendPingMessage(0, seqNumber++);
}

void FileClient::sendNextFilePart()
{
	if (state != sendingFS)
	{
		cerr << "Unable to send FS state != sendingFS!" << endl;
		return;
	}

	bool wsRefreshed = false;

	if (!curWorkingSet)
	{
		curWorkingSet = fS->getWorkingSet();
		printToDo();
		wsRefreshed = true;
	}

	// Continue file transfer:
	if (curWorkingSet->curFilePartNr >= 0)
	{
		bool lastPartSend = sendNextFilePart(curWorkingSet->curFile.first, curWorkingSet->curFile.second, ++curWorkingSet->curFilePartNr, uploadClient);
		if(lastPartSend) {
			curWorkingSet->files.erase(curWorkingSet->curFile.first);
			curWorkingSet->curFilePartNr = -1;
		}
	}
	// Trasfer folder:
	else if (!curWorkingSet->folders.empty())
	{
		struct Folder *f = curWorkingSet->folders.front();
		curWorkingSet->folders.pop_front();
		sendFolderCreationMessage(f, uploadClient);
	}
	// Transfer file:
	else if (!curWorkingSet->files.empty())
	{
		curWorkingSet->curFile = *curWorkingSet->files.begin();	
		curWorkingSet->curFilePartNr = 0;
		sendFileCreationMessage(curWorkingSet->curFile.first, curWorkingSet->curFile.second, uploadClient);
	}
	else
	{
		cout << "Transfer finished!" << endl;
		state = connected;
		return;
	}

	// ToDo: send next file part...
	state = awaitingAck;
}

void FileClient::sendFolderCreationMessage(struct Folder *f, Client *client)
{
	const char *c = f->path.c_str();
	uint64_t l = strlen(c);
	int i = getNextSeqNumber();
	FileCreationMessage msg = FileCreationMessage(clientId, i, 1, NULL, l, (unsigned char *)c);
	sendMessageQueue->pushSendMessage(i, msg);

	client->send(&msg);
	cout << "Send folder: " << f->path << endl;
}

void FileClient::sendFileCreationMessage(string fid, struct File *f, Client *client)
{
	const char *c = fid.c_str();
	uint64_t l = strlen(c);
	int i = getNextSeqNumber();
	FileCreationMessage msg = FileCreationMessage(clientId, i, 4, (unsigned char *)f->hash, l, (unsigned char *)c);
	sendMessageQueue->pushSendMessage(i, msg);

	client->send(&msg);
	cout << "Send file creation: " << fid << endl;
}

bool FileClient::sendNextFilePart(string fid, struct File *f, int nextPartNr, Client *client) {
	char chunk[Filesystem::partLength];
	bool isLastPart = false;
	int readCount = fS->readFile(fid, chunk, nextPartNr, &isLastPart);

	// File part:
	char flags = 2;

	// First file part:
	if(nextPartNr == 0) {
		flags |= 1;
	}
	// Last file part:
	else if(isLastPart){
		flags |= 8;
	}

	int i = getNextSeqNumber();
	FileTransferMessage msg = FileTransferMessage(clientId, i, flags, nextPartNr, (unsigned char *)f->hash, (uint64_t)readCount, (unsigned char*)chunk);
	sendMessageQueue->pushSendMessage(i, msg);

	client->send(&msg);
	cout << "Send file part " << nextPartNr << " for file: " << fid << endl;
	return isLastPart;
}

unsigned int FileClient::getNextSeqNumber() {
	unique_lock<mutex> mlock(*seqNumberMutex);
	unsigned int i = seqNumber++;
	mlock.unlock();
	return i;
}

void FileClient::onTransferEndedMessage(net::ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, TransferEndedMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	switch (state)
	{
	case sendingFS:
	case awaitingAck:
	case connected:
		cerr << "Received TransferEndedMessage but client is not connected or sending the FS!" << endl;
		return;
	}

	unsigned char flags = TransferEndedMessage::getFlagsFromMessage(msg->buffer);
	cout << "Received TransferEndedMessage with flags: ";
	AbstractMessage::printByte(flags);
	cout << endl
		 << "Stopping file transfer client..." << endl;
	stopSendingFS();
	cout << "Stoped file transfer client!" << endl;
}

void FileClient::pingServer()
{
	switch (state)
	{
	case connected:
	case awaitingAck:
	case sendingFS:
		cout << "Ping" << endl;
		sendPingMessage(0, getNextSeqNumber(), uploadClient);
		break;

	default:
		cerr << "Unable to ping server - not connected!" << endl;
		break;
	}
}

TransferState FileClient::getState()
{
	return state;
}

void FileClient::sendClientHelloMessage(unsigned short listeningPort, Client *client)
{
	ClientHelloMessage msg = ClientHelloMessage(listeningPort, clientId);
	client->send(&msg);
}

void FileClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber, Client *client)
{
	PingMessage msg = PingMessage(plLength, seqNumber, clientId);
	sendMessageQueue->pushSendMessage(seqNumber, msg);

	client->send(&msg);
}

void FileClient::stopSendingFS()
{
	shouldTransferRun = false;
	stopConsumerThread();
	stopHelloThread();
	server->stop();
}

void FileClient::printToDo()
{
	cout << "ToDo list:" << endl;
	if(!curWorkingSet) {
		cout << "Nothing to do!" << endl;
	}
	else {
		cout << "Files: " << curWorkingSet->files.size() << endl;
		cout << "Folders: " << curWorkingSet->folders.size() << endl;
		cout << "Current file: ";
		if(curWorkingSet->curFilePartNr >= 0) {
			cout << "FID: " << curWorkingSet->curFile.first << ", Part: " << curWorkingSet->curFilePartNr << endl;
		}
		else {
			cout << "None" << endl;
		}
	}
}