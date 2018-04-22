#pragma once
#include "net/AbstractMessage.h"

namespace net {
	class ClientHelloMessage : public AbstractMessage {
	public:
		ClientHelloMessage();
		void toByteArray();
	};
}