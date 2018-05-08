#include "FileClient.h"

using namespace net;
using namespace std;

FileClient::FileClient(std::string* serverAddress, unsigned short serverPort, FilesystemClient* fs) {
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->fs = fs;
	this->state = disconnected;
	this->cpQueue = new Queue<ReadMessage>();
	this->shouldConsumerRun = false;
	this->shouldTransferRun = false;
	this->shouldHelloRun = false;
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
	startHelloThread(listenPort);
}

void FileClient::startHelloThread(unsigned short listenPort) {
	shouldHelloRun = true;
	helloThread = new thread(&FileClient::helloTask, this, listenPort);
}
void FileClient::stopHelloThread() {
	shouldHelloRun = false;
	if(helloThread && helloThread->joinable()) {
		helloThread->join();
	}
}

void FileClient::helloTask(unsigned short listenPort) {
	state = sendHandshake;
	while(state != connected && shouldHelloRun) {
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

void FileClient::onServerHelloMessage(ReadMessage *msg) {
	// Check if the checksum of the received package is valid else drop it:
	if(!AbstractMessage::isChecksumValid(msg, ServerHelloMessage::CHECKSUM_OFFSET_BITS)) {
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
		// cout << msg.msgType << endl;
		switch(msg.msgType) {
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

void FileClient::onAckMessage(ReadMessage *msg) {
	// Check if the checksum of the received message is valid else drop it:
	if(!AbstractMessage::isChecksumValid(msg, AckMessage::CHECKSUM_OFFSET_BITS)) {
		return;
	}
	
	if(state != awaitingAck) {
		cerr << "Received ACK message but state is not awaitingAck!" << endl;
		return;
	}
	
	unsigned int seq = AckMessage::getSeqNumberFromMessage(msg->buffer);
	cout << "Ack: " << seq << endl;

	// Update state:
	state = sendingFS;

	sendNextFilePart();
	// sendPingMessage(0, seqNumber++);
}

void FileClient::sendNextFilePart() {
	if(state != sendingFS) {
		cerr << "Unable to send FS state != sendingFS!" << endl;
		return;
	}

	// ToDo: send next file part...
	state = awaitingAck;
}

void FileClient::onTransferEndedMessage(net::ReadMessage *msg) {
	// Check if the checksum of the received message is valid else drop it:
	if(!AbstractMessage::isChecksumValid(msg, TransferEndedMessage::CHECKSUM_OFFSET_BITS)) {
		return;
	}

	switch(state) {
		case sendingFS:
		case awaitingAck:
		case connected:
			cerr << "Received TransferEndedMessage but client is not connected or sending the FS!" << endl;
			return;
	}

	unsigned char flags = TransferEndedMessage::getFlagsFromMessage(msg->buffer);
	cout << "Received TransferEndedMessage with flags: ";
	AbstractMessage::printByte(flags);
	cout << endl << "Stopping file transfer client..." << endl;
	stopSendingFS();
	cout << "Stoped file transfer client!" << endl; 
}

void FileClient::pingServer() {
	switch(state) {
		case connected:
		case awaitingAck:
		case sendingFS:
			cout << "Ping" << endl;
			sendPingMessage(0, seqNumber++);
			break;
		
		default:
			cerr << "Unable to ping server - not connected!" << endl;
			break;
	}
}

TransferState FileClient::getState() {
	return state;
}

void FileClient::sendClientHelloMessage(unsigned short listeningPort) {
	ClientHelloMessage msg = ClientHelloMessage(listeningPort);
	client.send(&msg);
}

void FileClient::sendPingMessage(unsigned int plLength, unsigned int seqNumber) {
	PingMessage msg = PingMessage(plLength, seqNumber);
	client.send(&msg);
}

void FileClient::transferFiles() {
	state = sendingFS;
	// ToDo: File transfer logic

	// setitimer https://linux.die.net/man/2/setitimer for detecting server timeouts

	sendPingMessage(0, seqNumber++);
	
	/*while(shouldTransferRun) {
		sendPingMessage(0, seqNumber++);
		sleep(1); // Sleep for 1s
	}*/
}

void FileClient::stopSendingFS() {
	shouldTransferRun = false;
	stopConsumerThread();
	stopHelloThread();
	server.stop();
}