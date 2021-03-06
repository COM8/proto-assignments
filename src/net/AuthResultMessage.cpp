#include "AuthResultMessage.h"

using namespace net;

AuthResultMessage::AuthResultMessage(unsigned int clientId, unsigned char flags, unsigned int seqNumber) : AbstractMessage(AUTH_RESULT_MESSAGE_ID << 4) {
	this->clientId = clientId;
	this->flags = flags;
	this->seqNumber = seqNumber;
}

void AuthResultMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[13]{};
	msg->bufferLength = 13;
	
	// Add type:
	msg->buffer[0] |= type;

    // Add flags:
	msg->buffer[0] |= flags;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	// Add Sequence number:
	setBufferUnsignedInt(msg, seqNumber, 40);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int AuthResultMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned char AuthResultMessage::getFlagsFromMessage(unsigned char* buffer) {
	return buffer[0] & 0xF; // 4 bit offset
}

unsigned int AuthResultMessage::getSeqNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 40);
}