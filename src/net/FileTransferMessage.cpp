#include "FileTransferMessage.h"

using namespace net;
using namespace std;

FileTransferMessage::FileTransferMessage(unsigned int clientId, unsigned int seqNumber, unsigned char flags, unsigned int fidPartNumber, unsigned char* fileHash, uint64_t contentLength, unsigned char* content) : AbstractMessage(3 << 4) { // 00110000
	this->clientId = clientId;
	this->seqNumber = seqNumber;
	this->flags = flags;
	this->fileHash = fileHash;
	this->contentLength = contentLength;
	this->content = content;
	this->fidPartNumber = fidPartNumber;
}

void FileTransferMessage::createBuffer(struct Message* msg) {
	msg->buffer = new unsigned char[57 + contentLength]{};
	msg->bufferLength = 57 + contentLength;
	
	// Add type:
	msg->buffer[0] |= type;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 4);

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 36);

	// Add flags:
	setByteWithOffset(msg, flags, 64); // Starts at 68 - ensure the first 4 bit are 0

	// Add FID part number:
	setBufferUnsignedInt(msg, fidPartNumber, 72);	

	// Add file hash:
	setBufferValue(msg, fileHash, 32, 104);

	// Add content length:
	setBufferUint64_t(msg, contentLength, 392);

	// Add content:
	setBufferValue(msg, content, contentLength, 456);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int FileTransferMessage::getClientIdFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 4);
}

unsigned int FileTransferMessage::getSeqNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 36);
}

unsigned char FileTransferMessage::getFlagsFromMessage(unsigned char* buffer) {
	// Flags start at 68, but it is easier to get a byte and ignore the first 4 bit
	return getByteWithOffset(buffer, 64) & 0xF; // Only the firts 4 bit are the flag bits
}

unsigned int FileTransferMessage::getFIDPartNumberFromMessage(unsigned char* buffer) {
	return getUnsignedIntFromMessage(buffer, 72);	
}

unsigned char* FileTransferMessage::getFileHashFromMessage(unsigned char* buffer) {
	return getBytesWithOffset(buffer, 104, 256);
}

uint64_t FileTransferMessage::getContentLengthFromMessage(unsigned char* buffer) {
	return getUnsignedInt64FromMessage(buffer, 392);
}

unsigned char* FileTransferMessage::getContentFromMessage(unsigned char* buffer, uint64_t fIDLength) {
	return getBytesWithOffset(buffer, 456, fIDLength * 8);
}