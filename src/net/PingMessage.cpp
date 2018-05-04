#include "PingMessage.h"

using namespace net;
using namespace std;

PingMessage::PingMessage(unsigned int plLength, unsigned int seqNumber) : AbstractMessage(6 << 4) { // 01100000
	this->plLength = plLength;
	this->seqNumber = seqNumber;
}

void PingMessage::createBuffer(struct Message* msg) {
	msg->buffer = new char[12 + plLength]{};
	msg->bufferLength = 12 + plLength;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add payload length:
	char plLengthArray[4];
    for (int i = 0; i < 4; i++) {
		plLengthArray[3 - i] = (plLength >> (i * 8));
    }
	setBufferValue(msg, plLengthArray, 4, 72);

	// Add sequence number:
	char seqNumberArray[4];
    for (int i = 0; i < 4; i++) {
		seqNumberArray[3 - i] = (seqNumber >> (i * 8));
    }
	setBufferValue(msg, seqNumberArray, 4, 4);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int PingMessage::getPlLengthFromMessage(unsigned char* buffer) {
	unsigned char* plLengthArray = AbstractMessage::getBytesWithOffset(buffer, 72, 32);
	return static_cast<int>(plLengthArray[0]) << 24 | plLengthArray[1] << 16 | plLengthArray[2] << 8 | plLengthArray[3];
}

unsigned int PingMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	unsigned char* seqNumberArray = AbstractMessage::getBytesWithOffset(buffer, 4, 32);
	return static_cast<int>(seqNumberArray[0]) << 24 | seqNumberArray[1] << 16 | seqNumberArray[2] << 8 | seqNumberArray[3];
}