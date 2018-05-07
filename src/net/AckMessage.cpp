#include "AckMessage.h"

using namespace net;
using namespace std;

AckMessage::AckMessage(unsigned int seqNumber) : AbstractMessage(5 << 4) { // 01010000
	this->seqNumber = seqNumber;
}

void AckMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[9]{};
	msg->bufferLength = 9;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 4);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int AckMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 36);
}