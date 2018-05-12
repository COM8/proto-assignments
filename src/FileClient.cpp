#include "FileClient.h"

using namespace net;
using namespace std;

FileClient::FileClient(std::string *serverAddress, unsigned short serverPort, FilesystemClient *fS)
{
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->fS = fS;
	this->curWorkingSet = fS->getWorkingSet();
	this->state = disconnected;
	this->cpQueue = new Queue<ReadMessage>();
	this->sendMessageQueue = new SendMessageQueue();
	this->shouldConsumerRun = false;
	this->shouldTransferRun = false;
	this->shouldHelloRun = false;
	this->consumerThread = NULL;
	this->seqNumber = 0;
	srand(time(NULL));
	this->clientId = rand();
	this->listeningPort = 2000 + rand() % 63000;
	this->server = NULL;
	this->client = NULL;
	this->uploadClient = NULL;
	this->seqNumberMutex = new mutex();
	this->sendMessageTimer = new Timer(true, 1000, this, ACK_TIMER_IDENT);
	this->sendMessageTimer->start();
	this->pingTimer = new Timer(true, 1000, this, PING_TIMER_IDENT);
	this->transferFinished = false;
}

void FileClient::onTimerTick(int identifier)
{
	switch (identifier)
	{
	case ACK_TIMER_IDENT:
	{
		// cout << "Tick " << sendMessageQueue->size() << endl;
		list<struct SendMessage> *msgs = new list<struct SendMessage>();
		sendMessageQueue->popNotAckedMessages(MAX_ACK_TIME_IN_S, msgs);

		if (state == disconnected)
		{
			// cout << "Discariding " << msgs->size() << "unacked messages!" << endl;
			return;
		}

		for (struct SendMessage msg : *msgs)
		{
			if (msg.sendCount >= 3)
			{
				restartSendingFS();
			}
			else
			{
				// Resend message:
				cout << "Resending message!" << endl;
				uploadClient->send(msg.msg);
				msg.sendCount++;
				msg.sendTime = time(NULL);
				sendMessageQueue->push(msg);
			}
		}
	}
	break;

	case PING_TIMER_IDENT:
		switch (state)
		{
		case connected:
			setState(ping);
			sendPingMessage(0, getNextSeqNumber(), uploadClient);
			cout << "Keep alive ping send." << endl;
			break;
		}
		break;
	}
}

void FileClient::restartSendingFS()
{
	cout << "Restarting file transfer." << endl;
	// Stop:
	setState(disconnected);

	// Switch listen port to prevent binding problems:
	listeningPort = 2000 + rand() % 63000;

	// Start:
	server = new Server(listeningPort, cpQueue);
	server->start();
	startConsumerThread();
	startHelloThread();
}

void FileClient::startSendingFS()
{
	shouldTransferRun = true;

	client = new Client(*serverAddress, serverPort);

	server = new Server(listeningPort, cpQueue);
	server->start();
	startConsumerThread();
	startHelloThread();

	printToDo();
}

void FileClient::startHelloThread()
{
	shouldHelloRun = true;
	helloThread = new thread(&FileClient::helloTask, this, listeningPort);
}
void FileClient::stopHelloThread()
{
	shouldHelloRun = false;
	if (helloThread && helloThread->joinable() && helloThread->get_id() != this_thread::get_id())
	{
		helloThread->join();
	}
}

void FileClient::helloTask(unsigned short listenPort)
{
	setState(sendHandshake);
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
	if (consumerThread && consumerThread->joinable() && consumerThread->get_id() != this_thread::get_id())
	{
		ReadMessage *msg = new ReadMessage();
		msg->msgType = 0xff;
		cpQueue->push(*msg);
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
		setState(connected);
		// Check if client ID was still available:
		if (ServerHelloMessage::getFlagsFromMessage(msg->buffer) & 1 != 1)
		{
			cout << "Client ID already taken! Trying a new one." << endl;
			clientId = rand();
			setState(disconnected);
			return;
		}

		cout << "Connected to server!" << endl;
		clientId = ServerHelloMessage::getClientIdFromMessage(msg->buffer);
		unsigned short uploadPort = ServerHelloMessage::getPortFromMessage(msg->buffer);
		uploadClient = new Client(*serverAddress, uploadPort);

		setState(sendingFS);
	}
}

TransferState FileClient::getState()
{
	return state;
}

void FileClient::setState(TransferState state)
{
	if (this->state == state)
	{
		return;
	}
	this->state = state;

	switch (state)
	{
	case disconnected:
		pingTimer->stop();
		sendMessageTimer->stop();
		sendMessageQueue->clear();
		stopConsumerThread();
		stopHelloThread();
		server->stop();
		if(shouldTransferRun) {
			startHelloThread();
		}		
		break;

	case sendHandshake:
		break;

	case connected:
		stopHelloThread();
		pingTimer->start();
		sendMessageTimer->start();
		if (!transferFinished && shouldTransferRun)
		{
			startSendingFS();
		}
		break;

	case sendingFS:
		pingTimer->stop();
		if (!transferFinished && shouldTransferRun)
		{
			sendNextFilePart();
		}
		break;

	case awaitingAck:
		break;

	case ping:
		break;
	}
}

void FileClient::consumerTask()
{
	while (shouldConsumerRun)
	{
		ReadMessage msg = cpQueue->pop();
		if (!shouldConsumerRun)
		{
			return;
		}

		switch (msg.msgType)
		{
		case 2:
			onServerHelloMessage(&msg);
			break;

		case 5:
			cout << "Ack" << endl;
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
		pingTimer->reset();
		// cout << "Acked: " << seqNumber << endl;
	}
	else
	{
		cerr << "Sequence number not found: " << seqNumber << endl;
	}

	switch (state)
	{
	case awaitingAck:
		setState(sendingFS);
		break;

	case ping:
		setState(connected);
		break;
	}
}

void FileClient::sendNextFilePart()
{
	if (state != sendingFS)
	{
		cerr << "Unable to send FS state != sendingFS!" << endl;
		return;
	}

	auto folders = curWorkingSet->getFolders();
	auto files = curWorkingSet->getFiles();

	// Continue file transfer:
	int curFilePartNr = curWorkingSet->getCurFilePartNr();
	if (curFilePartNr >= 0)
	{
		auto curFile = curWorkingSet->getCurFile();
		setState(awaitingAck);
		bool lastPartSend = sendNextFilePart(curFile->first, curFile->second, curFilePartNr, uploadClient);

		curWorkingSet->setCurFilePartNr(++curFilePartNr);

		if (lastPartSend)
		{
			files->erase(curFile->first);
			curWorkingSet->setCurFilePartNr(-1);
		}
		curWorkingSet->unlockCurFile();
	}
	// Trasfer folder:
	else if (!folders->empty())
	{
		struct Folder *f = folders->front();
		folders->pop_front();
		setState(awaitingAck);
		sendFolderCreationMessage(f, uploadClient);
	}
	// Transfer file:
	else if (!files->empty())
	{
		pair<string, File *> *nextCurFile = new pair<string, File *>(files->begin()->first, files->begin()->second);
		curWorkingSet->setCurFile(nextCurFile);
		curWorkingSet->setCurFilePartNr(0);

		auto curFile = curWorkingSet->getCurFile();
		setState(awaitingAck);
		sendFileCreationMessage(curFile->first, curFile->second, uploadClient);
		curWorkingSet->unlockCurFile();
	}
	else
	{
		curWorkingSet->unlockFiles();
		curWorkingSet->unlockdelFolders();
		sendTransferEndedMessage(0b0001, uploadClient);
		transferFinished = true;
		cout << "Transfer finished!" << endl;
		setState(disconnected);
		return;
	}

	curWorkingSet->unlockFiles();
	curWorkingSet->unlockFolders();
}

void FileClient::sendTransferEndedMessage(unsigned char flags, Client *client)
{
	TransferEndedMessage *msg = new TransferEndedMessage(clientId, flags);
	client->send(msg);
}

void FileClient::sendFolderCreationMessage(struct Folder *f, Client *client)
{
	const char *c = f->path.c_str();
	uint64_t l = f->path.length();
	int i = getNextSeqNumber();
	FileCreationMessage *msg = new FileCreationMessage(clientId, i, 1, NULL, l, (unsigned char *)c);
	sendMessageQueue->pushSendMessage(i, msg);

	client->send(msg);
	cout << "Send folder creation: \"" << f->path << "\"" << endl;
}

void FileClient::sendFileCreationMessage(string fid, struct File *f, Client *client)
{
	const char *c = fid.c_str();
	uint64_t l = fid.length();
	int i = getNextSeqNumber();
	FileCreationMessage *msg = new FileCreationMessage(clientId, i, 4, (unsigned char *)f->hash, l, (unsigned char *)c);
	sendMessageQueue->pushSendMessage(i, msg);

	client->send(msg);
	cout << "Send file creation: " << fid << endl;
}

bool FileClient::sendNextFilePart(string fid, struct File *f, unsigned int nextPartNr, Client *client)
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

	int i = getNextSeqNumber();
	FileTransferMessage *msg = new FileTransferMessage(clientId, i, flags, nextPartNr, (unsigned char *)f->hash, (uint64_t)readCount, (unsigned char *)chunk);
	sendMessageQueue->pushSendMessage(i, msg);

	client->send(msg);
	cout << "Send file part " << nextPartNr << ", length: " << readCount << " for file: " << fid << endl;
	return isLastPart;
}

unsigned int FileClient::getNextSeqNumber()
{
	unique_lock<mutex> mlock(*seqNumberMutex);
	unsigned int i = seqNumber++;
	mlock.unlock();
	return i;
}

void FileClient::onTransferEndedMessage(ReadMessage *msg)
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
	case ping:
		cout << "Ping" << endl;
		sendPingMessage(0, getNextSeqNumber(), uploadClient);
		break;

	default:
		cerr << "Unable to ping server - not connected!" << endl;
		break;
	}
}

void FileClient::sendClientHelloMessage(unsigned short listeningPort, Client *client)
{
	ClientHelloMessage *msg = new ClientHelloMessage(listeningPort, clientId);
	client->send(msg);
}

void FileClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber, Client *client)
{
	PingMessage *msg = new PingMessage(plLength, seqNumber, clientId);
	sendMessageQueue->pushSendMessage(seqNumber, msg);

	client->send(msg);
}

void FileClient::stopSendingFS()
{
	switch (state)
	{
	case sendingFS:
	case awaitingAck:
	case connected:
		sendTransferEndedMessage(0b0010, uploadClient);
		break;
	}
	shouldTransferRun = false;
	setState(disconnected);
}

void FileClient::printToDo()
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
		cout << "Delete files: " << curWorkingSet->getdelFiles()->size() << endl;
		cout << "Delete folders: " << curWorkingSet->getdelFolders()->size() << endl;
		cout << "Current file: ";
		if (curWorkingSet->getCurFilePartNr() >= 0)
		{
			cout << "FID: " << curWorkingSet->getCurFile()->first << ", Part: " << curWorkingSet->getCurFilePartNr() << endl;
		}
		else
		{
			cout << "None" << endl;
		}

		// Unlock workingset:
		curWorkingSet->unlockCurFile();
		curWorkingSet->unlockdelFiles();
		curWorkingSet->unlockdelFolders();
		curWorkingSet->unlockFiles();
		curWorkingSet->unlockFolders();
	}
}