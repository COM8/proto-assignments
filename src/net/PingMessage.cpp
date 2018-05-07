#include "PingMessage.h"

using namespace net;
using namespace std;

PingMessage::PingMessage(unsigned int plLength, unsigned int seqNumber) : AbstractMessage(6 << 4) { // 01100000
	this->plLength = plLength;
	this->seqNumber = seqNumber;
}

void PingMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[12 + plLength]{};
	msg->bufferLength = 12 + plLength;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add payload length:
	setBufferUnsignedInt(msg, plLength, 72);

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 4);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int PingMessage::getPlLengthFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 72);
}

unsigned int PingMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 4);
}