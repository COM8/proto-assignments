#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class TransferEndedMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 40;

		TransferEndedMessage(unsigned int clientId, unsigned char flags);
		virtual void createBuffer(struct Message* msg);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);

	private:
		unsigned int clientId;
		unsigned char flags;
	};
}