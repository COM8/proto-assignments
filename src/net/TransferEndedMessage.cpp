#include "TransferEndedMessage.h"

using namespace net;

TransferEndedMessage::TransferEndedMessage(unsigned int clientId, unsigned char flags) : AbstractMessage(7 << 4) { // 01110000
	this->clientId = clientId;
	this->flags = flags;
}

void TransferEndedMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[9]{};
	msg->bufferLength = 9;
	
	// Add type:
	msg->buffer[0] |= type;

    // Add flags:
	setByteWithOffset(msg, flags, 0); // Starts at 4 - ensure the first 4 bit are 0

	// Add client id:
	setBufferInt(msg, clientId, 8);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int TransferEndedMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned char TransferEndedMessage::getFlagsFromMessage(unsigned char* buffer) {
	return getByteWithOffset(buffer, 0) & 0xf;
}