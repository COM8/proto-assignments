#pragma once

#include "net/AbstractMessage.h"
#include <cstdint>

namespace net
{

class FileTransferMessage : public AbstractMessage
{
  public:
	static const unsigned int CHECKSUM_OFFSET_BITS = 360;

	FileTransferMessage(unsigned int clientId, unsigned int seqNumber, unsigned char flags, unsigned int fidPartNumber, unsigned char *fileHash, uint64_t contentLength, unsigned char *content);
	virtual void createBuffer(struct Message *msg);
	static unsigned int getClientIdFromMessage(unsigned char *buffer);
	static unsigned int getSeqNumberFromMessage(unsigned char *buffer);
	static unsigned int getFIDPartNumberFromMessage(unsigned char *buffer);
	static unsigned char getFlagsFromMessage(unsigned char *buffer);
	static unsigned char *getFileHashFromMessage(unsigned char *buffer);
	static uint64_t getContentLengthFromMessage(unsigned char *buffer);
	static unsigned char *getContentFromMessage(unsigned char *buffer, uint64_t fIDLength);

  private:
	unsigned int clientId;
	unsigned int seqNumber;
	unsigned int fidPartNumber;
	unsigned char flags;
	unsigned char *fileHash;
	uint64_t contentLength;
	unsigned char *content;
};
}