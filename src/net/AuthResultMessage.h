#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class AuthResultMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 40;

		AuthResultMessage(unsigned int clientId, unsigned char flags);
		void createBuffer(struct Message* msg);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);

	private:
		unsigned int clientId;
		unsigned char flags;
	};
}