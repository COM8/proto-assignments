#include "net/ClientHelloMessage.h"

using namespace net;
using namespace std;

ClientHelloMessage::ClientHelloMessage(unsigned short port) : AbstractMessage(1 << 4) { // 00010000
	this->port = port;
}

void ClientHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[7]{};
	msg->bufferLength = 7;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add port:
	setBufferUnsignedShort(msg, port, 4);

	//cout << "Pre: " << port << endl;
	//AbstractMessage::printMessage(msg);
	//cout << "set: " << endl;
	setBufferUnsignedInt(msg, 0xffffffff, CHECKSUM_OFFSET_BITS);
	// AbstractMessage::printByteArray(msg->buffer, msg->bufferLength);

	// Add checksum:
	//addChecksum(msg, 20);
}

unsigned short ClientHelloMessage::getPortFromMessage(unsigned char* buffer) {
	return getUnsignedShortFromMessage(buffer, 16);
}
