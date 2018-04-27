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
		cout << "New client accepted!" << endl;

		client = new FileClientConnection {};
		client->clientId = clientId++;
		client->state = c_clientHello;
		client->portRemote = 1235; // Get from received message
		//client->remoteIp = msg.senderIp;
		client->portLocal = 1235; // Get random one
	}
}

void FileServer::stop() {
	stopConsumerThread();
}