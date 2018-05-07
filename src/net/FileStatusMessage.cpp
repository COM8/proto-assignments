#include "FileStatusMessage.h"

using namespace net;
using namespace std;

FileStatusMessage::FileStatusMessage(unsigned int clientId, unsigned int lastSeqNumber, unsigned char flags, uint64_t fIDLength, unsigned char* fID) : AbstractMessage(4 << 4) { // 01000000
	this->clientId = clientId;
	this->lastSeqNumber = lastSeqNumber;
	this->flags = flags;
	this->fIDLength = fIDLength;
	this->fID = fID;
}

void FileStatusMessage::createBuffer(struct Message* msg) {
	msg->buffer = new char[53 + fIDLength]{};
	msg->bufferLength = 53 + fIDLength;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	setByteWithOffset(msg, flags, 0); // Starts at 4 - ensure the first 4 bit are 0

	// Add client id:
	setBufferInt(msg, clientId, 8);

	// Add last sequence number:
	setBufferInt(msg, lastSeqNumber, 40);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);

	// Add content length:
	setBufferUint64_t(msg, fIDLength, 104);

	// Add content:
	setBufferValue(msg, fID, fIDLength, 168);
}

unsigned int FileStatusMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned int FileStatusMessage::getLastSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 40);
}

unsigned char FileStatusMessage::getFlagsFromMessage(unsigned char* buffer) {
	return getByteWithOffset(buffer, 0) & 0xf;
}

uint64_t FileStatusMessage::getFIDLengthFromMessage(unsigned char* buffer) {
	return getUnsignedInt64FromMessage(buffer, 104);
}

unsigned char* FileStatusMessage::getFIDFromMessage(unsigned char* buffer, uint64_t fIDLength) {
	return getBytesWithOffset(buffer, 168, fIDLength * 8);
}