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
	setBufferValue(msg, &flags, 1, 4);

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	// Add port:
	setBufferUnsignedShort(msg, port, 40);

	// Add checksum:
	addChecksum(msg, 56);
}

unsigned char ServerHelloMessage::getFlagsFromMessage(unsigned char* buffer) {
	// Flags start at 4, but it is easier to get a byte and ignore the first 4 bit
	return getByteWithOffset(buffer, 0) & 0xF; // Only the last 4 bit are the flag bits
}

unsigned int ServerHelloMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned short ServerHelloMessage::getPortFromMessage(unsigned char* buffer) {
	return getUnsignedShortFromMessage(buffer, 40);
}