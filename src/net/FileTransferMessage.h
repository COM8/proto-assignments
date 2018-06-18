#pragma once

#include <cstdint>
#include "net/AbstractMessage.h"

namespace net
{

class FileTransferMessage : public AbstractMessage
{
public:
	static const unsigned int CHECKSUM_OFFSET_BITS = 104;

	FileTransferMessage(unsigned int clientId, unsigned int seqNumber, unsigned char flags, unsigned int fidPartNumber, uint64_t contentLength, unsigned char *content);
	virtual void createBuffer(struct Message *msg);
	static unsigned int getClientIdFromMessage(unsigned char *buffer);
	static unsigned int getSeqNumberFromMessage(unsigned char *buffer);
	static unsigned int getFIDPartNumberFromMessage(unsigned char *buffer);
	static unsigned char getFlagsFromMessage(unsigned char *buffer);
	static uint64_t getContentLengthFromMessage(unsigned char *buffer);
	static unsigned char *getContentFromMessage(unsigned char *buffer, uint64_t fIDLength);

private:
	unsigned int clientId;
	unsigned int seqNumber;
	unsigned int fidPartNumber;
	unsigned char flags;
	uint64_t contentLength;
	unsigned char *content;
};
}