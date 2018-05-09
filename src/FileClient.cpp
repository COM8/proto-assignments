#include "FileClient.h"

using namespace net;
using namespace std;

FileClient::FileClient(std::string *serverAddress, unsigned short serverPort, FilesystemClient *fs)
{
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->fs = fs;
	this->curWorkingSet = NULL;
	this->state = disconnected;
	this->cpQueue = new Queue<ReadMessage>();
	this->sendMessageQueue = new SendMessageQueue();
	this->shouldConsumerRun = false;
	this->shouldTransferRun = false;
	this->shouldHelloRun = false;
	this->consumerThread = NULL;
	this->seqNumber = 0;
	this->clientId = -1;
	this->server = NULL;
	this->client = NULL;
	this->uploadClient = NULL;
}

void FileClient::startSendingFS()
{
	shouldTransferRun = true;

	client = new Client(*serverAddress, serverPort);

	unsigned short listenPort = 1235;
	server = new Server(listenPort, cpQueue);
	server->start();
	startConsumerThread();
	startHelloThread(listenPort);
}

void FileClient::startHelloThread(unsigned short listenPort)
{
	shouldHelloRun = true;
	helloThread = new thread(&FileClient::helloTask, this, listenPort);
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
		sleep(1); // Sleep for 1s
		sendClientHelloMessage(listenPort, client);
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
		cout << "Ack: " << seqNumber << endl;
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
		fs->genMap();
		curWorkingSet = fs->getWorkingSet();
		wsRefreshed = true;
	}

	// Continue file transfer:
	if (curWorkingSet->curFilePartNr >= 0)
	{
		cout << "cFile" << endl;
		bool lastPartSend = sendNextFilePart(curWorkingSet->curFile.first, curWorkingSet->curFile.second, ++curWorkingSet->curFilePartNr, uploadClient);
		if(lastPartSend) {
			curWorkingSet->files->erase(curWorkingSet->curFile.first);
			curWorkingSet->curFilePartNr = -1;
		}
	}
	// Trasfer folder:
	else if (!curWorkingSet->folders->empty())
	{
		struct Folder *f = curWorkingSet->folders->front();
		curWorkingSet->folders->pop_front();
	}
	// Transfer file:
	else if (!curWorkingSet->files->empty())
	{
		curWorkingSet->curFile = *curWorkingSet->files->begin();	
		curWorkingSet->curFilePartNr = 0;
		sendFileCreationMessage(curWorkingSet->curFile.first, curWorkingSet->curFile.second, uploadClient);
		curWorkingSet->curFilePartNr = -1;
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
	unsigned char *c = (unsigned char *)f->path.c_str();
	FileCreationMessage msg = FileCreationMessage(clientId, seqNumber++, 1, NULL, (uint64_t)f->path.length(), c);
	sendMessageQueue->pushSendMessage(seqNumber - 1, msg);

	client->send(&msg);
	cout << "Send folder: " << f->path << endl;
}

void FileClient::sendFileCreationMessage(string fid, struct File *f, Client *client)
{
	unsigned char *c = (unsigned char *)fid.c_str();
	FileCreationMessage msg = FileCreationMessage(clientId, seqNumber++, 4, (unsigned char *)f->hash, (uint64_t)fid.length(), c);
	sendMessageQueue->pushSendMessage(seqNumber - 1, msg);

	client->send(&msg);
	cout << "Send file creation: " << fid << endl;
}

bool FileClient::sendNextFilePart(string fid, struct File *f, int nextPartNr, Client *client) {
	char chunk[MAX_FILE_CHUNK_SIZE_IN_BYTE];
	int readCount = fs->readFile(fid, chunk, nextPartNr, MAX_FILE_CHUNK_SIZE_IN_BYTE);

	char flags = 1;
	if(nextPartNr == 0) {
		flags = 1;
	}
	else if(readCount == -1){
		flags = 8;
	}
	else {
		flags = 2;
	}

	FileTransferMessage msg = FileTransferMessage(clientId, seqNumber++, 0, (unsigned char *)f->hash, (uint64_t)readCount, (unsigned char*)chunk);
	client->send(&msg);
	return true;
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
		sendPingMessage(0, seqNumber++, uploadClient);
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
	ClientHelloMessage msg = ClientHelloMessage(listeningPort);
	client->send(&msg);
}

void FileClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber, Client *client)
{
	PingMessage msg = PingMessage(plLength, seqNumber);
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
		cout << "Files: " << curWorkingSet->files->size() << endl;
		cout << "Folders: " << curWorkingSet->folders->size() << endl;
		cout << "Current file: ";
		if(curWorkingSet->curFilePartNr >= 0) {
			cout << "FID: " << curWorkingSet->curFile.first << ", Part: " << curWorkingSet->curFilePartNr << endl;
		}
		else {
			cout << "None" << endl;
		}
	}
}