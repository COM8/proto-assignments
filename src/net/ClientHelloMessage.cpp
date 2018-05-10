#include "net/ClientHelloMessage.h"

using namespace net;
using namespace std;

ClientHelloMessage::ClientHelloMessage(unsigned short port, unsigned int clientId) : AbstractMessage(1 << 4) { // 00010000
	this->port = port;
	this->clientId = clientId;
}

void ClientHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[11]{};
	msg->bufferLength = 11;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add port:
	setBufferUnsignedShort(msg, port, 4);

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 20);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned short ClientHelloMessage::getPortFromMessage(unsigned char* buffer) {
	return getUnsignedShortFromMessage(buffer, 4);
}

unsigned int ClientHelloMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 20);
}
