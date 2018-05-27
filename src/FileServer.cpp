#include "FileServer.h"

using namespace net;
using namespace std;

FileServer::FileServer(unsigned short port) : port(port),
											  state(stopped),
											  shouldConsumerRun(false)
{
	this->cpQueue = new Queue<ReadMessage>();
	this->clients = new unordered_map<unsigned int, FileClientConnection *>();
	this->cleanupTimer = new Timer(true, 5000, this, 0);
	this->clientsMutex = new std::mutex();
}

void FileServer::onTimerTick(int identifier)
{
	cleanupClients();
}

void FileServer::start()
{
	if (state != stopped)
	{
		cerr << "Unable to start file server! State != stopped" << endl;
	}
	else
	{
		state = starting;
		server = Server(port, cpQueue);
		server.start();
		startConsumerThread();
		cleanupTimer->start();
		state = running;
	}
}

void FileServer::onTransferEndedMessage(net::ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, TransferEndedMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = TransferEndedMessage::getClientIdFromMessage(msg->buffer);
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		unsigned char flags = TransferEndedMessage::getFlagsFromMessage(msg->buffer);
		// Transfer finished:
		if ((flags & 0b0001) == 0b0001)
		{
			cout << "Client " << c->second->clientId << " finished transfer. Removing client." << endl;
			disconnectClient(c->second);
			clients->erase(c->second->clientId);
		}
		// Calceled by user:
		else if ((flags & 0b0010) == 0b0010)
		{
			cout << "Client " << c->second->clientId << " canceled transfer. Removing client." << endl;
			disconnectClient(c->second);
			clients->erase(c->second->clientId);
		}
		// Error:
		else
		{
			cout << "Client " << c->second->clientId << " transfer failed! Removing client." << endl;
			disconnectClient(c->second);
			clients->erase(c->second->clientId);
		}
	}
	mlock.unlock();
}

void FileServer::startConsumerThread()
{
	shouldConsumerRun = true;
	consumerThread = new thread(&FileServer::consumerTask, this);
}

void FileServer::stopConsumerThread()
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

void FileServer::consumerTask()
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
		case 1:
			onClientHelloMessage(&msg);
			break;

		case 2:
			onFileCreationMessage(&msg);
			break;

		case 3:
			onFileTransferMessage(&msg);
			break;

		case 4:
			onFileStatusMessage(&msg);
			break;

		case 5:
			onAckMessage(&msg);
			break;

		case 6:
			onPingMessage(&msg);
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

void FileServer::onFileStatusMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, FileStatusMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned char flags = FileStatusMessage::getFlagsFromMessage(msg->buffer);
	if ((flags & 0b0001) != 0b0001)
	{
		cout << "Invalid flags for file status message received: " << (int)flags << endl;
		return;
	}

	unsigned int clientId = FileStatusMessage::getClientIdFromMessage(msg->buffer);
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		uint64_t fidLengt = FileStatusMessage::getFIDLengthFromMessage(msg->buffer);
		unsigned char *fid = FileStatusMessage::getFIDFromMessage(msg->buffer, fidLengt);
		string fidString = string((char *)fid, fidLengt);
		unsigned int lastPart = c->second->fS->getLastPart(fidString);
		unsigned int seqNumber = FileStatusMessage::getSeqNumberFromMessage(msg->buffer);

		unsigned char recFlags = FileStatusMessage::getFlagsFromMessage(msg->buffer);
		unsigned char flags = 0b0010;
		flags |= (recFlags & 0b1000);

		FileStatusMessage *fSMsg = new FileStatusMessage(clientId, seqNumber, lastPart, flags, c->second->curFIDLength, (unsigned char *)c->second->curFID.c_str());
		c->second->udpClient->send(fSMsg);
	}

	mlock.unlock();
}

void FileServer::onFileCreationMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, FileCreationMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = FileCreationMessage::getClientIdFromMessage(msg->buffer);
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		// Send ack:
		unsigned int seqNumber = FileCreationMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage *ack = new AckMessage(seqNumber);
		FileClientConnection *fCC = c->second;
		fCC->udpClient->send(ack);

		fCC->lastMessageTime = time(NULL);

		// Create file/folder:
		unsigned char fileType = FileCreationMessage::getFileTypeFromMessage(msg->buffer);
		uint64_t fidLengt = FileCreationMessage::getFIDLengthFromMessage(msg->buffer);
		unsigned char *fid = FileCreationMessage::getFIDFromMessage(msg->buffer, fidLengt);
		unsigned char *hash;
		string fidString = string((char *)fid, fidLengt);
		switch (fileType)
		{
		case 1:
			fCC->fS->genFolder(fidString);
			cout << "Folder \"" << fidString << "\" generated." << endl;
			break;

		case 2:
			fCC->fS->delFolder(fidString);
			break;

		case 4:
			hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
			fCC->curFID = fidString;
			fCC->curFIDLength = fidLengt;
			fCC->fS->genFile(fCC->curFID, (char *)hash);
			cout << "File \"" << fid << "\" generated." << endl;
			break;

		case 8:
			fCC->fS->delFile(fidString);
			break;

		default:
			cerr << "Unknown fileType received: " << fileType << endl;
			break;
		}
	}
	mlock.unlock();
}

void FileServer::onFileTransferMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, FileTransferMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = FileTransferMessage::getClientIdFromMessage(msg->buffer);
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		// Send ack:
		unsigned int seqNumber = FileTransferMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage *ack = new AckMessage(seqNumber);
		FileClientConnection *fCC = c->second;
		fCC->udpClient->send(ack);

		fCC->lastMessageTime = time(NULL);

		// Write file:
		char flags = FileTransferMessage::getFlagsFromMessage(msg->buffer);
		if ((flags & 0b10) == 0b10)
		{
			unsigned int partNumber = FileTransferMessage::getFIDPartNumberFromMessage(msg->buffer);
			uint64_t contLength = FileTransferMessage::getContentLengthFromMessage(msg->buffer);
			unsigned char *content = FileTransferMessage::getContentFromMessage(msg->buffer, contLength);
			int result = fCC->fS->writeFilePart(fCC->curFID, (char *)content, partNumber, contLength);
			cout << "Wrote file part: " << partNumber << ", length: " << contLength << " for file: \"" << fCC->curFID << "\" with result: " << result << endl;
			if ((flags & 0b1000) == 0b1000)
			{
				cout << "Last filepart recieved: " << fCC->curFID << endl;
			}
		}
		else
		{
			cerr << "Invalid FileTransferMessage flags received: " << (int)flags << endl;
		}
	}
	mlock.unlock();
}

void FileServer::onAckMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, AckMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}
	cout << "Ack" << endl;
}

void FileServer::onPingMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, PingMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = PingMessage::getClientIdFromMessage(msg->buffer);
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		c->second->lastMessageTime = time(NULL);
		unsigned int seqNumber = PingMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		cout << "Pong" << endl;
		c->second->udpClient->send(&ack);
	}
	mlock.unlock();
}

void FileServer::cleanupClients()
{
	time_t now = time(NULL);

	std::unique_lock<std::mutex> mlock(*clientsMutex);
	try
	{
		auto i = clients->begin();
		while (i != clients->end())
		{
			FileClientConnection *c = i->second;
			if (c && difftime(now, c->lastMessageTime) > 10)
			{
				cout << "Removing client: " << c->clientId << " for inactivity." << endl;

				disconnectClient(c);
				i = clients->erase(i);
			}
			else
			{
				i++;
			}
		}
	}
	catch (...)
	{
	}
	mlock.unlock();
}

void FileServer::disconnectClient(FileClientConnection *client)
{
	client->state = c_disconnected;
	if (client->fS)
	{
		client->fS->close();
	}
	if (client->udpServer)
	{
		client->udpServer->stop();
	}
}

void FileServer::onClientHelloMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, ClientHelloMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned char flags = ClientHelloMessage::getFlagsFromMessage(msg->buffer);
	if ((flags & 0b0001) != 0b0001)
	{
		cout << "Client hello message without \"connect requested\" received: " << (int)flags << endl;
		return;
	}

	FileClientConnection *client = new FileClientConnection{};
	client->clientId = ClientHelloMessage::getClientIdFromMessage(msg->buffer);
	client->state = c_clientHello;
	client->portRemote = ClientHelloMessage::getPortFromMessage(msg->buffer);
	client->remoteIp = msg->senderIp;
	client->portLocal = 2000 + client->clientId % 63000;
	client->cpQueue = cpQueue;
	client->udpClient = new Client(client->remoteIp, client->portRemote);
	client->udpServer = new Server(client->portLocal, client->cpQueue);
	client->fS = new FilesystemServer("sync/" + to_string(client->clientId) + "/");
	client->curFID = "";
	client->lastMessageTime = time(NULL);
	client->lastFIDPartNumber = 0;
	client->curFIDLength = 0;

	// Check if client id is taken:
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	auto c = clients->find(client->clientId);
	if ((flags & 0b0010) == 0b0010 && c != clients->end())
	{
		cout << "Client reconnect request received." << endl;
		if (c->second && c->second->state != c_disconnected)
		{
			disconnectClient(c->second);
		}
	}
	else if (c != clients->end() && c->second && c->second->state != c_disconnected)
	{
		sendServerHelloMessage(client, 4);
		cout << "Client request declined! Reason: Client ID already taken!" << endl;
		return;
	}

	(*clients)[client->clientId] = client;
	client->udpServer->start();
	sendServerHelloMessage(client, 1);
	client->state = c_serverHello;

	cout << "New client with id: " << client->clientId << " accepted on port: " << client->portRemote << endl;

	mlock.unlock();
}

void FileServer::sendServerHelloMessage(FileClientConnection *client, unsigned char flags)
{
	ServerHelloMessage *msg = new ServerHelloMessage(client->portLocal, client->clientId, flags);
	client->udpClient->send(msg);
}

void FileServer::stop()
{
	stopConsumerThread();
	cleanupTimer->stop();

	// Stop all clients:
	std::unique_lock<std::mutex> mlock(*clientsMutex);
	for (auto itr = clients->begin(); itr != clients->end(); itr++)
	{
		FileClientConnection *c = itr->second;
		cout << "Stoping client: " << c->clientId << endl;
		disconnectClient(c);
	}
	mlock.unlock();
}