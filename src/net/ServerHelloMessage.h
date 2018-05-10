#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class ServerHelloMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 56;

		ServerHelloMessage(unsigned short port, unsigned int clientId, unsigned char flags);
		void createBuffer(struct Message* msg);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned short getPortFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);

	private:
		unsigned short port;
		unsigned int clientId;
		unsigned char flags; // Only the fist 4 bits from the left are used!
	};
}