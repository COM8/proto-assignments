#pragma once
#include "net/AbstractMessage.h"

namespace net {
	
	class ServerHelloMessage : public AbstractMessage {
	public:
		ServerHelloMessage(unsigned short port, unsigned int clientId, char flags);
		void createBuffer(struct Message* msg);

	private:
		unsigned short port;
		unsigned int clientId;
		char flags; // Only the fist 4 bits from the left are used!
	};
}