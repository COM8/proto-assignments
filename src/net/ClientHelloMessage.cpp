#include "net/ClientHelloMessage.h"

using namespace net;
using namespace std;

ClientHelloMessage::ClientHelloMessage(unsigned short port) : AbstractMessage(1 << 4) { // 00010000
	this->port = port;
}

void ClientHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[5]{};
	msg->bufferLength = 5;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add port:
	setBufferShort(msg, port, 4);

	// Add checksum:
	addChecksum(msg, 20);
}

unsigned short ClientHelloMessage::getPortFromMessage(unsigned char* buffer) {
	unsigned char* portArray = AbstractMessage::getBytesWithOffset(buffer, 4, 16);
	return static_cast<int16_t>(portArray[0]) << 8 | portArray[1];
}
