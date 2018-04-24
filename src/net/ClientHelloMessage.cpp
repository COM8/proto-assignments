#include "net/ClientHelloMessage.h"

using namespace net;
using namespace std;

ClientHelloMessage::ClientHelloMessage(unsigned short port) : AbstractMessage(1 << 4) { // 00010000
	this->port = port;
}

void ClientHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new char[5];
	msg->bufferLength = 5;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add port:
	char port16[2];
	port16[0] = (port >> 8) & 0xFF;
	port16[1] = port & 0xFF;
	setBufferValue(msg, port16, 2, 4);

	// Add checksum:
	addChecksum(msg, 20);

	// Print result:
	// printMessage(msg);
}
