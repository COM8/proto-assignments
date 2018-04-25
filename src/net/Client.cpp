#include "net/Client.h"

using namespace std;
using namespace net;

Client::Client(string hostAddr, unsigned short port) {
	this->hostAddr = hostAddr;
	this->port = port;
	this->sockFD = -1;
	this->state = stopped;
	this->clientThread = NULL;
	this->shouldRun = false;
}

void Client::setState(State state) {
	// ToDo: Trigger some kind of event.
	//cout << "UDP server new state: " << state << endl;
	this->state = state;
}

State Client::getState() {
	return state;
}

void Client::stop() {
	setState(stopping);
	shouldRun = false;
	if(clientThread && clientThread->joinable()) {
		clientThread->join();
	}
	setState(stopped);
}

void Client::start() {
	if(clientThread) {
		cerr << "UDP client already running! Please stop first!" << endl;
	}
	else {
		// Start a new server thread:
		shouldRun = true;
		thread t1 = thread(&Client::startTask, this);
		// Keep server thread running even if got out of focus:
		t1.detach();

		clientThread = &t1;
	}
}

void Client::startTask() {
	if(sockFD > 0) {
		cerr << "UDP client already running! Please stop first!" << endl;
	}
	else if((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cerr << "UDP socket creation failed!" << endl;
		setState(error);
	}
	else {
		setState(starting);

		serverAddressStruct = {};
		serverAddressStruct.sin_family = AF_INET;
		serverAddressStruct.sin_port = htons(port);

		// Look up the address of the server given its name:
		struct hostent* hp = {};
		hp = gethostbyname(hostAddr.c_str());
		if (!hp) {
			cerr << "UDP client unable to look up host: " << hostAddr << endl;
		 	setState(error);
		}
		else {
			// Add host address to the address structure:
			memcpy((void *)&serverAddressStruct.sin_addr, hp->h_addr_list[0], hp->h_length);
			setState(running);
		}
	}
}

bool Client::send(AbstractMessage* msg) {
	struct Message msgStruct = {};
	msg->createBuffer(&msgStruct);

	// Print message:
	printMessage(&msgStruct);
	
	if (sendto(sockFD, msgStruct.buffer, msgStruct.bufferLength, 0, (struct sockaddr *)&serverAddressStruct, sizeof(serverAddressStruct)) < 0) {
		cerr << "UDP client failed to send message!" << endl;
		return false;
	}
	return true;
}