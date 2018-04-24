#include "FileClient.h"

using namespace net;

FileClient::FileClient(std::string* serverAddress, unsigned short serverPort, Filesystem* fs) {
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->fs = fs;
	this->state = disconnected;
}

void FileClient::startSendingFS() {
	client = Client(*serverAddress, serverPort);
	client.start();

	unsigned short listenPort = 1235;

	while(state == disconnected) {
		usleep(1000); // Sleep for 1s
		sendClientHelloMessage(listenPort);
	}
}

void FileClient::sendClientHelloMessage(unsigned short listeningPort) {
	ClientHelloMessage msg = ClientHelloMessage(listeningPort);
	client.send(&msg);
}

void FileClient::stopSendingFS() {

}