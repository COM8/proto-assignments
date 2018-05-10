#include "FileServer.h"

using namespace net;
using namespace std;

FileServer::FileServer(unsigned short port) : port(port),
											  clientId(0),
											  state(stopped),
											  shouldConsumerRun(false)
{
	this->cpQueue = new Queue<ReadMessage>();
	this->clients = new unordered_map<unsigned int, FileClientConnection*>();
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
	if(c != clients->end()) {
		// Send ack:
		unsigned int seqNumber = FileCreationMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		FileClientConnection * fCC = c->second;
		fCC->udpClient->send(&ack);

		// Create file/folder:
		unsigned char fileType = FileCreationMessage::getFileTypeFromMessage(msg->buffer);
		uint64_t fidLengt = FileCreationMessage::getFIDLengthFromMessage(msg->buffer);
		unsigned char *fid = FileCreationMessage::getFIDFromMessage(msg->buffer, fidLengt);
		unsigned char *hash;
		switch(fileType) {
			case 1:
				fCC->fS->genFolder(string((char*)fid));
				break;

			case 2:
				fCC->fS->delFolder(string((char*)fid));
				break;

			case 4:
				hash = FileCreationMessage::getFileHashFromMessage(msg->buffer);
				fCC->curFID = string((char*)fid);
				fCC->fS->genFile(fCC->curFID, (char*)hash);
				break;

			case 8:
				fCC->fS->delFile(string((char*)fid));
				break;

			default:
				cerr << "Unknown fileType received: " << fileType << endl;
				break;
		}
	}
}

void FileServer::onFileTransferMessage(ReadMessage *msg) {
	// Check if the checksum of the received message is valid else drop it:
	if (!AbstractMessage::isChecksumValid(msg, FileCreationMessage::CHECKSUM_OFFSET_BITS))
	{
		return;
	}

	unsigned int clientId = FileTransferMessage::getClientIdFromMessage(msg->buffer);
	auto c = clients->find(clientId);
	if(c != clients->end()) {
		// Send ack:
		unsigned int seqNumber = FileTransferMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		FileClientConnection * fCC = c->second;
		fCC->udpClient->send(&ack);

		// Write file:
		unsigned int partNumber = FileTransferMessage::getFIDPartNumberFromMessage(msg->buffer);
		uint64_t contLength = FileTransferMessage::getContentLengthFromMessage(msg->buffer);
		unsigned char *content = FileTransferMessage::getContentFromMessage(msg->buffer, contLength);
		fCC->fS->writeFilePart(fCC->curFID, (char*)content, partNumber, contLength);
		cout << "Wrote file part: " << partNumber << " for file: " << fCC->curFID << endl;
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
	if(c != clients->end()) {
		unsigned int seqNumber = FileCreationMessage::getSeqNumberFromMessage(msg->buffer);
		AckMessage ack(seqNumber);
		c->second->udpClient->send(&ack);
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
	client->clientId = clientId++;
	client->state = c_clientHello;
	client->portRemote = ClientHelloMessage::getPortFromMessage(msg->buffer);
	client->remoteIp = msg->senderIp;
	client->portLocal = 1236; // ToDo: Get random unused port
	client->cpQueue = new Queue<ReadMessage>();
	client->udpClient = new Client(client->remoteIp, client->portRemote);
	client->udpServer = new Server(client->portLocal, client->cpQueue);
	client->udpServer->start();
	client->fS = new FilesystemServer("" + clientId);
	client->curFID = "";

	// Insert client into clients map:
	(*clients)[client->clientId] = client;

	sendServerHelloMessage(*client);
	client->state = c_serverHello;

	cout << "New client accepted on port: " << client->portRemote << " and clientId: " << client->clientId << endl;
}

void FileServer::sendServerHelloMessage(const FileClientConnection &client)
{
	ServerHelloMessage msg(client.portLocal, client.clientId, 1 << 4);
	client.udpClient->send(&msg);
}

void FileServer::stop()
{
	stopConsumerThread();

	// Stop all clients:
	for (auto itr = clients->begin(); itr != clients->end(); itr++) {
		FileClientConnection *c = itr->second;
		cout << "Stoping client: " << c->clientId << endl;
		if(c->fS) {
			c->fS->close();
		}
		if(c->udpServer) {
			c->udpServer->stop();
		}
	}
}