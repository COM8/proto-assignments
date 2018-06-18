#include "FileTransferMessage.h"

using namespace net;
using namespace std;

FileTransferMessage::FileTransferMessage(unsigned int clientId, unsigned int seqNumber, unsigned char flags, unsigned int fidPartNumber, uint64_t contentLength, unsigned char *content) : AbstractMessage(FILE_TRANSFER_MESSAGE_ID << 4)
{
	this->clientId = clientId;
	this->seqNumber = seqNumber;
	this->flags = flags;
	this->contentLength = contentLength;
	this->content = content;
	this->fidPartNumber = fidPartNumber;
}

void FileTransferMessage::createBuffer(struct Message *msg)
{
	msg->buffer = new unsigned char[25 + contentLength]{};
	msg->bufferLength = 25 + contentLength;

	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 40);

	// Add FID part number:
	setBufferUnsignedInt(msg, fidPartNumber, 72);

	// Add content length:
	setBufferUint64_t(msg, contentLength, 136);

	// Add content:
	setBufferValue(msg, content, contentLength, 200);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int FileTransferMessage::getClientIdFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned int FileTransferMessage::getSeqNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 40);
}

unsigned char FileTransferMessage::getFlagsFromMessage(unsigned char *buffer)
{
	return buffer[0] & 0xF; // 4 bit offset
}

unsigned int FileTransferMessage::getFIDPartNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 72);
}

uint64_t FileTransferMessage::getContentLengthFromMessage(unsigned char *buffer)
{
	return getUnsignedInt64FromMessage(buffer, 136);
}

unsigned char *FileTransferMessage::getContentFromMessage(unsigned char *buffer, uint64_t fIDLength)
{
	return getBytesWithOffset(buffer, 200, fIDLength * 8);
}