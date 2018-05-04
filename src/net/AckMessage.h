#pragma once

#include "net/AbstractMessage.h"

namespace net {
	
	class AckMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 36;

		AckMessage(unsigned int seqNumber);
		void createBuffer(struct Message* msg);
		static unsigned int getSeqNumberFromMessage(unsigned char* buffer);

	private:
		unsigned int seqNumber;
	};
}