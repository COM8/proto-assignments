#include "FileClient.h"

using namespace net;
using namespace std;

FileClient::FileClient(std::string* serverAddress, unsigned short serverPort, Filesystem* fs) {
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->fs = fs;
	this->state = disconnected;
	this->cpQueue = new Queue<ReadMessage>();
	this->shouldConsumerRun = false;
	this->shouldTransferRun = false;
	this->consumerThread = NULL;
	this->seqNumber = 0;
}

void FileClient::startSendingFS() {
	shouldTransferRun = true;

	client = Client(*serverAddress, serverPort);

	unsigned short listenPort = 1235;
	server = Server(listenPort, cpQueue);
	server.start();
	startConsumerThread();
	
	state = sendHandshake;
	while(state != connected) {
		sleep(1); // Sleep for 1s
		sendClientHelloMessage(listenPort);
	}
}

void FileClient::startConsumerThread() {
	shouldConsumerRun = true;
	consumerThread = new thread(&FileClient::consumerTask, this);
}

void FileClient::stopConsumerThread() {
	shouldConsumerRun = false;
	if(consumerThread && consumerThread->joinable()) {
		consumerThread->join();
	}
}

void FileClient::onServerHelloMessage(ReadMessage& msg) {
	// Check if the checksum of the received package is valid else drop it:
	if(!AbstractMessage::isChecksumValid(&msg, ServerHelloMessage::CHECKSUM_OFFSET_BITS)) {
		return;
	}

	if(state == sendHandshake) {
		state = connected;
		cout << "Connected to server!" << endl;

		transferFiles();
	}
}

void FileClient::consumerTask() {
	while(shouldConsumerRun) {
		ReadMessage msg = cpQueue->pop();
		switch(msg.msgType) {
			case 2:
				onServerHelloMessage(msg);
				break;

			default:
				cerr << "Unknown message type received: " << msg.msgType << endl;
				break;
		}
	}
}

void FileClient::sendClientHelloMessage(unsigned short listeningPort) {
	ClientHelloMessage msg = ClientHelloMessage(listeningPort);
	client.send(&msg);
}

void FileClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber) {
	PingMessage msg = PingMessage(plLength, seqNumber);
	client.send(&msg);
	cout << "Ping" << endl;
}

void FileClient::transferFiles() {
	// ToDo: File transfer logic
	while(shouldTransferRun) {
		sendPingMessage(0, seqNumber++);
		sleep(1); // Sleep for 1s
	}
}

void FileClient::stopSendingFS() {
	shouldTransferRun = false;
	stopConsumerThread();
	server.stop();
}