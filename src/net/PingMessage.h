#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class PingMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 68;

		PingMessage(unsigned int plLength, unsigned int seqNumber, unsigned int clientId);
		void createBuffer(struct Message* msg);
		static unsigned int getPlLengthFromMessage(unsigned char* buffer);
		static unsigned int getSeqNumberFromMessage(unsigned char* buffer);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);

	private:
		unsigned int plLength;
		unsigned int seqNumber;
		unsigned int clientId;
	};
}