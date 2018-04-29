#include "FileServer.h"

using namespace net;
using namespace std;

FileServer::FileServer(unsigned short port) : port(port),
											  clientId(0),
											  state(stopped),
											  shouldConsumerRun(false),
											  client(NULL) {
	cpQueue = new Queue<ReadMessage>();
}

void FileServer::start() {
	if(state != stopped) {
		cerr << "Unable to start file server! State != stopped" << endl;
	}
	else {
		state = starting;
		server = Server(port, cpQueue);
		server.start();
		startConsumerThread();
		state = running;
	}
}

void FileServer::startConsumerThread() {
	shouldConsumerRun = true;
	consumerThread = new thread(&FileServer::consumerTask, this);
}

void FileServer::stopConsumerThread() {
	shouldConsumerRun = false;
	if(consumerThread && consumerThread->joinable()) {
		consumerThread->join();
	}
}

void FileServer::consumerTask() {
	while(shouldConsumerRun) {
		ReadMessage msg = cpQueue->pop();
		switch(msg.msgType) {
			case 1:
				onClientHelloMessage(msg);
				break;

			default:
				cerr << "Unknown message type received: " << msg.msgType << endl;
				break;
		}
	}
}

void FileServer::onClientHelloMessage(ReadMessage &msg) {
	if(!client) {
		client = new FileClientConnection {};
		client->clientId = clientId++;
		client->state = c_clientHello;
		client->portRemote = ClientHelloMessage::getPortFromMessage(msg.buffer);
		client->remoteIp = msg.senderIp;
		client->portLocal = 1235; // ToDo: Get random one
		client->cpQueue = new Queue<ReadMessage>();
		client->udpClient = new Client(client->remoteIp, client->portRemote);
		client->udpServer = new Server(client->portLocal, client->cpQueue);
		client->udpServer->start();

		sendServerHelloMessage(*client);
		client->state = c_serverHello;

		cout << "New client accepted on port: " << client->portRemote << endl;
	}
}

void FileServer::sendServerHelloMessage(const FileClientConnection& client) {
	ServerHelloMessage msg(client.portLocal, client.clientId, 1 << 4);
	client.udpClient->send(&msg);
}

void FileServer::stop() {
	stopConsumerThread();
}