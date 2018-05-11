#include "FileServer.h"

using namespace net;
using namespace std;

FileServer::FileServer(unsigned short port) : port(port),
											  state(stopped),
											  shouldConsumerRun(false)
{
	this->cpQueue = new Queue<ReadMessage>();
	this->clients = new unordered_map<unsigned int, FileClientConnection *>();
	this->cleanupTimer = new Timer(true, 5000, this);
}

void FileServer::onTimerTick() {
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

void FileServer::startConsumerThread()
{
	shouldConsumerRun = true;
	consumerThread = new thread(&FileServer::consumerTask, this);
}

void FileServer::stopConsumerThread()
{
	shouldConsumerRun = false;
	if (consumerThread && consumerThread->joinable())
	{
		consumerThread->join();
	}
}

void FileServer::consumerTask()
{
	while (shouldConsumerRun)
	{
		ReadMessage msg = cpQueue->pop();
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

			/*case 4:
			break;*/

		case 5:
			onAckMessage(&msg);
			break;

		case 6:
			onPingMessage(&msg);
			break;

			/*case 7:
			break;*/

		default:
			cerr << "Unknown message type received: " << msg.msgType << endl;
			break;
		}
	}
}

void FileServer::onFileCreationMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, FileCreationMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = FileCreationMessage::getClientIdFromMessage(msg->buffer);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		// Send ack:
		unsigned int seqNumber = FileCreationMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		FileClientConnection *fCC = c->second;
		fCC->udpClient->send(&ack);

		// Create file/folder:
		unsigned char fileType = FileCreationMessage::getFileTypeFromMessage(msg->buffer);
		uint64_t fidLengt = FileCreationMessage::getFIDLengthFromMessage(msg->buffer);
		unsigned char *fid = FileCreationMessage::getFIDFromMessage(msg->buffer, fidLengt);
		unsigned char *hash;
		string fidString = string((char *)fid, fidLengt);
		switch (fileType)
		{
		case 1:
			cout << "dsfdsad" << endl;
			fCC->fS->genFolder(fidString);			
			cout << "Folder \"" << fidString << "\" generated." << endl;
			break;

		case 2:
			fCC->fS->delFolder(fidString);
			break;

		case 4:
			hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
			fCC->curFID = fidString;
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
}

void FileServer::onFileTransferMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, FileTransferMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = FileTransferMessage::getClientIdFromMessage(msg->buffer);
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		// Send ack:
		unsigned int seqNumber = FileTransferMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		FileClientConnection *fCC = c->second;
		fCC->udpClient->send(&ack);

		// Write file:
		char flags = FileTransferMessage::getFlagsFromMessage(msg->buffer);
		if ((flags & 2) == 2)
		{
			unsigned int partNumber = FileTransferMessage::getFIDPartNumberFromMessage(msg->buffer);
			uint64_t contLength = FileTransferMessage::getContentLengthFromMessage(msg->buffer);
			unsigned char *content = FileTransferMessage::getContentFromMessage(msg->buffer, contLength);
			fCC->fS->writeFilePart(fCC->curFID, (char *)content, partNumber, contLength);
			cout << "Wrote file part: " << partNumber << " for file: " << fCC->curFID << endl;
			if((flags & 8) == 8) {
				cout << "File ende received: " << fCC->curFID << endl;
			}
		}
		else
		{
			cerr << "Invalid FileTransferMessage flags received: " << (int)flags << endl;
		}
	}
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
	auto c = clients->find(clientId);
	if (c != clients->end())
	{
		unsigned int seqNumber = PingMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		cout << "Pong" << endl;
		c->second->udpClient->send(&ack);
	}
}

void FileServer::cleanupClients()
{
	time_t now = time(NULL);
	for (auto itr = clients->begin(); itr != clients->end();)
	{
		FileClientConnection *c = itr->second;
		if (difftime(now, c->lastMessageTime) > 10)
		{
			cout << "Removing client: " << c->clientId << " for inactivity." << endl;
			if (c->fS)
			{
				c->fS->close();
			}
			if (c->udpServer)
			{
				c->udpServer->stop();
			}
			itr = clients->erase(itr);
		}
		else
		{
			itr++;
		}
	}
}

void FileServer::onClientHelloMessage(ReadMessage *msg)
{
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, ClientHelloMessage::CHECKSUM_OFFSET_BITS))
	{
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
	client->fS = new FilesystemServer(to_string(client->clientId)+"/");
	client->curFID = "";
	(client->lastMessageTime) = time(NULL);

	// Check if client id is taken:
	auto c = clients->find(client->clientId);
	if (c != clients->end())
	{
		sendServerHelloMessage(client, 4);
		cout << "Client request declined! Reason: Client ID already taken!" << endl;
		return;
	}

	// Insert client into clients map:
	clients->insert(std::pair<int, FileClientConnection *>(client->clientId, client));

	client->udpServer->start();
	sendServerHelloMessage(client, 1);
	client->state = c_serverHello;

	cout << "New client with id: " << client->clientId << " accepted on port: " << client->portRemote << " and clientId: " << client->clientId << endl;
}

void FileServer::sendServerHelloMessage(FileClientConnection *client, unsigned char flags)
{
	ServerHelloMessage msg(client->portLocal, client->clientId, flags);
	client->udpClient->send(&msg);
}

void FileServer::stop()
{
	stopConsumerThread();
	cleanupTimer->stop();

	// Stop all clients:
	for (auto itr = clients->begin(); itr != clients->end(); itr++)
	{
		FileClientConnection *c = itr->second;
		cout << "Stoping client: " << c->clientId << endl;
		if (c->fS)
		{
			c->fS->close();
		}
		if (c->udpServer)
		{
			c->udpServer->stop();
		}
	}
}