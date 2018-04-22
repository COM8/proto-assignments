#include "net/Server.h"

using namespace std;
using namespace net;

Server::Server(unsigned int port) {
	this->port = port;
	this->sockFD = -1;
	this->state = stopped;
}

void Server::setState(State state) {
	// ToDo: Trigger some kind of event.
	cout << "UDP server new state: " << state << endl;
	this->state = state;
}

State Server::getState() {
	return state;
}

void Server::stop() {
	setState(stopping);
	sockFD = -1;
	setState(stopped);
}

void Server::start() {
	if(sockFD > 0) {
		cerr << "UDP server already running! Please stop first!" << endl;
	}
	else if((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cerr << "UDP socket creation failed!" << endl;
		setState(error);
	}
	else {
		setState(starting);

		// Fill memory block with 0: 
		memset((char *)&myAddr, 0, sizeof(myAddr));

		myAddr.sin_family = AF_INET;
		myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myAddr.sin_port = htons(port);
		
		if (bind(sockFD, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
			cerr << "UDP server binding failed!" << endl;
			setState(error);
		}
		else {
			setState(running);
			contReadStart();
		}
	}
}

void Server::contReadStart() {
	int recvlen = -1;
	unsigned char buf[BUFSIZE];
	struct sockaddr_in remAddr;
	socklen_t addrLen = sizeof(remAddr);

	while(true) {
		recvlen = recvfrom(sockFD, buf, BUFSIZE, 0, (struct sockaddr *)&remAddr, &addrLen);
		printf("received %d bytes\n", recvlen);
		
		if (recvlen > 0) {
			buf[recvlen] = 0;
			printf("received message: %s\n", buf);
		}
	}
}

bool Server::send() {

}