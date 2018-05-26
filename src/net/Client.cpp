#include "net/Client.h"

using namespace std;
using namespace net;

Client::Client(string hostAddr, unsigned short port) {
	this->hostAddr = hostAddr;
	this->port = port;
	this->sockFD = -1;
	srand (time(NULL));
	init();
}

void Client::init() {
	if((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cerr << "UDP socket creation failed!" << endl;
	}
	else {
		serverAddressStruct = {};
		serverAddressStruct.sin_family = AF_INET;
		serverAddressStruct.sin_port = htons(port);

		// Look up the address of the server given its name:
		struct hostent* hp = {};
		hp = gethostbyname(hostAddr.c_str());
		if (!hp) {
			cerr << "UDP client unable to look up host: " << hostAddr << endl;
		}
		else {
			// Add host address to the address structure:
			memcpy((void*)&serverAddressStruct.sin_addr, hp->h_addr_list[0], hp->h_length);
		}
	}
}

bool Client::send(AbstractMessage* msg) {
	struct Message msgStruct = {};
	msg->createBuffer(&msgStruct);
	// Print message:
	// AbstractMessage::printMessage(&msgStruct);
	
	// Message drop chance:
	if(rand() % 100 + 1 <= MESSAGE_DROP_CHANCE) {
		cout << "Droped message!" << endl;
		return true;
	}


	if (sendto(sockFD, msgStruct.buffer, msgStruct.bufferLength, 0, (struct sockaddr *)&serverAddressStruct, sizeof(serverAddressStruct)) < 0) {
		cerr << "UDP client failed to send message to: " << hostAddr << " and port: " << port << endl;
		return false;
	}
	return true;
}