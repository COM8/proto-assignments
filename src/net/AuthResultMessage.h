#pragma once

#include "net/AbstractMessage.h"

namespace net
{

class AuthResultMessage : public AbstractMessage
{
  public:
	static const unsigned int CHECKSUM_OFFSET_BITS = 72;

	AuthResultMessage(unsigned int clientId, unsigned char flags, unsigned int seqNumber);
	virtual void createBuffer(struct Message *msg);
	static unsigned int getClientIdFromMessage(unsigned char *buffer);
	static unsigned char getFlagsFromMessage(unsigned char *buffer);
	static unsigned int getSeqNumberFromMessage(unsigned char *buffer);

  private:
	unsigned int clientId;
	unsigned char flags;
	unsigned int seqNumber;
};
}