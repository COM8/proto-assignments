#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class ClientHelloMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 20;

		ClientHelloMessage(unsigned short port);
		void createBuffer(struct Message* msg);
		static unsigned short getPortFromMessage(unsigned char* buffer);

	private:
		unsigned short port;
	};
}