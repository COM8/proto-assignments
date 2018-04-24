#pragma once
#include "net/AbstractMessage.h"

namespace net {
	
	class ClientHelloMessage : public AbstractMessage {
	public:
		ClientHelloMessage(unsigned short port);
		void createBuffer(struct Message* msg);

	private:
		unsigned short port;
	};
}