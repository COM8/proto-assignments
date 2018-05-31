#include "FileStatusMessage.h"

using namespace net;
using namespace std;

FileStatusMessage::FileStatusMessage(unsigned int clientId, unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID) : AbstractMessage(4 << 4)
{
	this->clientId = clientId;
	this->lastFIDPartNumber = lastFIDPartNumber;
	this->flags = flags;
	this->fIDLength = fIDLength;
	this->fID = fID;
	this->seqNumber = seqNumber;
}

FileStatusMessage::FileStatusMessage(unsigned int clientId, unsigned int seqNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID) : AbstractMessage(4 << 4)
{
	this->clientId = clientId;
	this->flags = flags;
	this->fIDLength = fIDLength;
	this->fID = fID;
	this->seqNumber = seqNumber;
}

void FileStatusMessage::createBuffer(struct Message *msg)
{
	msg->buffer = new unsigned char[25 + fIDLength]{};
	msg->bufferLength = 25 + fIDLength;

	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 40);

	if ((flags & 0b0010) == 0b0010)
	{
		// Add last FID part number:
		setBufferUnsignedInt(msg, lastFIDPartNumber, 72);
	}

	// Add FID length:
	setBufferUint64_t(msg, fIDLength, 136);

	// Add FID:
	setBufferValue(msg, fID, fIDLength, 200);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int FileStatusMessage::getSeqNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 40);
}

unsigned int FileStatusMessage::getClientIdFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned int FileStatusMessage::getLastFIDPartNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 72);
}

unsigned char FileStatusMessage::getFlagsFromMessage(unsigned char *buffer)
{
	return buffer[0] & 0xf;
}

uint64_t FileStatusMessage::getFIDLengthFromMessage(unsigned char *buffer)
{
	return getUnsignedInt64FromMessage(buffer, 136);
}

unsigned char *FileStatusMessage::getFIDFromMessage(unsigned char *buffer, uint64_t fIDLength)
{
	return getBytesWithOffset(buffer, 200, fIDLength * 8);
}