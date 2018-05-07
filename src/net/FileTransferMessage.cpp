#include "FileTransferMessage.h"

using namespace net;
using namespace std;

FileTransferMessage::FileTransferMessage(unsigned int clientId, unsigned int seqNumber,unsigned char fileType, unsigned char* fileHash, uint64_t contentLength, unsigned char* content) : AbstractMessage(3 << 4) { // 00110000
	this->clientId = clientId;
	this->seqNumber = seqNumber;
	this->fileType = fileType;
	this->fileHash = fileHash;
	this->contentLength = contentLength;
	this->content = content;
}

void FileTransferMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[53 + contentLength]{};
	msg->bufferLength = 53 + contentLength;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add client id:
	setBufferInt(msg, clientId, 4);

	// Add sequence number:
	setBufferInt(msg, seqNumber, 36);

	// Add file type:
	setByteWithOffset(msg, fileType, 64); // Starts at 68 - ensure the first 4 bit are 0

	// Add file hash:
	setBufferValue(msg, fileHash, 32, 72);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);

	// Add content length:
	setBufferUint64_t(msg, contentLength, 360);

	// Add content:
	setBufferValue(msg, content, contentLength, 424);
}

unsigned int FileTransferMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 36);
}

unsigned int FileTransferMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 4);
}

unsigned char FileTransferMessage::getFileTypeFromMessage(unsigned char* buffer) {
	// Flags start at 68, but it is easier to get a byte and ignore the first 4 bit
	return getByteWithOffset(buffer, 64) & 0xF; // Only the firts 4 bit are the flag bits
}

unsigned char* FileTransferMessage::getFileHashFromMessage(unsigned char* buffer) {
	return getBytesWithOffset(buffer, 72, 256);
}

uint64_t FileTransferMessage::getContentLengthFromMessage(unsigned char* buffer) {
	return getUnsignedInt64FromMessage(buffer, 360);
}

unsigned char* FileTransferMessage::getContentFromMessage(unsigned char* buffer, uint64_t fIDLength) {
	return getBytesWithOffset(buffer, 72, fIDLength * 8);
}