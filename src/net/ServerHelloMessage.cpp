#include "net/ServerHelloMessage.h"

using namespace net;
using namespace std;

ServerHelloMessage::ServerHelloMessage(unsigned short port, unsigned int clientId, unsigned char flags) : AbstractMessage(2 << 4) { // 00100000
	this->port = port;
	this->clientId = clientId;
	this->flags = flags;
}

void ServerHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[11]{};
	msg->bufferLength = 11;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	// Add port:
	setBufferUnsignedShort(msg, port, 40);

	// Add checksum:
	addChecksum(msg, 56);
}

unsigned char ServerHelloMessage::getFlagsFromMessage(unsigned char* buffer) {
	return buffer[0] & 0xF; // 4 bit offset
}

unsigned int ServerHelloMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned short ServerHelloMessage::getPortFromMessage(unsigned char* buffer) {
	return getUnsignedShortFromMessage(buffer, 40);
}