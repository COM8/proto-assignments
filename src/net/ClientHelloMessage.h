#pragma once
#include "net/AbstractMessage.h"

namespace net {
	class ClientHelloMessage : public AbstractMessage {
	public:
		ClientHelloMessage(unsigned int port);
		void createBuffer(struct Message* msg);

	private:
		unsigned int port;
	};
}