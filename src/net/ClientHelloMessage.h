#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class ClientHelloMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 56;

		ClientHelloMessage(unsigned short port, unsigned int clientId, unsigned char flags);
		void createBuffer(struct Message* msg);
		static unsigned short getPortFromMessage(unsigned char* buffer);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);

	private:
		unsigned short port;
		unsigned int clientId;
		unsigned char flags;
	};
}