#include "net/ServerHelloMessage.h"

using namespace net;
using namespace std;

ServerHelloMessage::ServerHelloMessage(unsigned short port, unsigned int clientId, char flags) : AbstractMessage(2 << 4) { // 00100000
	this->port = port;
	this->clientId = clientId;
	this->flags = flags;
}

void ServerHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new char[9]{};
	msg->bufferLength = 9;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	setBufferValue(msg, &flags, 1, 4);

	// Add clientId:
	char clientId32[2];
	clientId32[0] = (clientId >> 24) & 0xFF;
	clientId32[1] = (clientId >> 16) & 0xFF;
	clientId32[2] = (clientId >> 8) & 0xFF;
	clientId32[3] = clientId & 0xFF;
	setBufferValue(msg, clientId32, 2, 40);

	// Add port:
	char port16[2];
	port16[0] = (port >> 8) & 0xFF;
	port16[1] = port & 0xFF;
	setBufferValue(msg, port16, 2, 40);

	// Add checksum:
	addChecksum(msg, 56);

	// Print result:
	// printMessage(msg);
}