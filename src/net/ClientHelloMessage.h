#pragma once

#include <cstdint>
#include "AbstractMessage.h"

namespace net {
	
	class ClientHelloMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 152;

		ClientHelloMessage(unsigned short port, unsigned int clientId, unsigned char flags, unsigned long prime, unsigned long primRoot, unsigned long pubKey, unsigned char* userName, unsigned int usernameLength);
		virtual void createBuffer(struct Message* msg);
		static unsigned short getPortFromMessage(unsigned char* buffer);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);
		static unsigned long getPubKeyFromMessage(unsigned char* buffer);
		static unsigned long getPrimeNumberFromMessage(unsigned char* buffer);
		static unsigned long getPrimitiveRootFromMessage(unsigned char* buffer);
		static unsigned int getUsernameLengthFromMessage(unsigned char* buffer);
		static unsigned char* getUsernameFromMessage(unsigned char* buffer, unsigned int usernameLength);

	private:
		unsigned short port;
		unsigned int clientId;
		unsigned char flags;
		unsigned long prime;
		unsigned long primRoot;
		unsigned long pubKey;
		unsigned char* userName;
		unsigned int usernameLength;
	};
}
