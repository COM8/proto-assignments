#include "FileStatusMessage.h"

using namespace net;
using namespace std;

FileStatusMessage::FileStatusMessage(unsigned int clientId, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char *fID) : AbstractMessage(4 << 4)
{
	this->clientId = clientId;
	this->lastFIDPartNumber = lastFIDPartNumber;
	this->flags = flags;
	this->fIDLength = fIDLength;
	this->fID = fID;
}

FileStatusMessage::FileStatusMessage(unsigned int clientId, unsigned char flags)
{
	this->clientId = clientId;
	this->flags = flags;
}

void FileStatusMessage::createBuffer(struct Message *msg)
{
	msg->buffer = new unsigned char[53 + fIDLength]{};
	msg->bufferLength = 53 + fIDLength;

	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	if ((flags & 0b0010) == 0b0010)
	{
		// Add last FID part number:
		setBufferUnsignedInt(msg, lastFIDPartNumber, 40);

		// Add content length:
		setBufferUint64_t(msg, fIDLength, 104);

		// Add content:
		setBufferValue(msg, fID, fIDLength, 168);
	}

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int FileStatusMessage::getClientIdFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned int FileStatusMessage::getLastFIDPartNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 40);
}

unsigned char FileStatusMessage::getFlagsFromMessage(unsigned char *buffer)
{
	return buffer[0] & 0xf;
}

uint64_t FileStatusMessage::getFIDLengthFromMessage(unsigned char *buffer)
{
	return getUnsignedInt64FromMessage(buffer, 104);
}

unsigned char *FileStatusMessage::getFIDFromMessage(unsigned char *buffer, uint64_t fIDLength)
{
	return getBytesWithOffset(buffer, 168, fIDLength * 8);
}