#include "FileCreationMessage.h"

using namespace net;
using namespace std;

FileCreationMessage::FileCreationMessage(unsigned int clientId, unsigned int seqNumber,unsigned char fileType, unsigned char* fileHash, uint64_t fIDLength, unsigned char* fID) : AbstractMessage(6 << 4) { // 01100000
	this->clientId = clientId;
	this->seqNumber = seqNumber;
	this->fileType = fileType;
	this->fileHash = fileHash;
	this->fIDLength = fIDLength;
	this->fID = fID;
}

void FileCreationMessage::createBuffer(struct Message* msg) {
	/*msg->buffer = new char[53 + plLength]{};
	msg->bufferLength = 53 + plLength;
	
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
	addChecksum(msg, CHECKSUM_OFFSET_BITS);*/
}

unsigned int FileCreationMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 36);
}

unsigned int FileCreationMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 4);
}

unsigned char FileCreationMessage::getFileTypeFromMessage(unsigned char* buffer) {
	// Flags start at 68, but it is easier to get a byte and ignore the first 4 bit
	return getByteWithOffset(buffer, 64) & 0xF; // Only the firts 4 bit are the flag bits
}

unsigned char* FileCreationMessage::getFileHashFromMessage(unsigned char* buffer) {
	return getBytesWithOffset(buffer, 72, 256);
}

uint64_t FileCreationMessage::getFIDLengthFromMessage(unsigned char* buffer) {
	return getUnsignedInt64FromMessage(buffer, 360);
}

unsigned char* FileCreationMessage::getFIDFromMessage(unsigned char* buffer, uint64_t fIDLength) {
	return getBytesWithOffset(buffer, 72, fIDLength * 8);
}