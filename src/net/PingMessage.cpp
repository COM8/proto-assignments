#include "PingMessage.h"

using namespace net;
using namespace std;

PingMessage::PingMessage(unsigned int plLength, unsigned int seqNumber, unsigned int clientId) : AbstractMessage(6 << 4) { // 01100000
	this->plLength = plLength;
	this->seqNumber = seqNumber;
}

void PingMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[12 + plLength]{};
	msg->bufferLength = 12 + plLength;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 4);

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 36);

	// Add payload length:
	setBufferUnsignedInt(msg, plLength, 104);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int PingMessage::getPlLengthFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 72);
}

unsigned int PingMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 4);
}

unsigned int PingMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 8);
}