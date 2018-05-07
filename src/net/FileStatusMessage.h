#pragma once

#include "net/AbstractMessage.h"
#include <cstdint>

namespace net {
	
	class FileStatusMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 72;

		FileStatusMessage(unsigned int clientId, unsigned int lastSeqNumber, unsigned char flags, uint64_t fIDLength, unsigned char* fID);
		void createBuffer(struct Message* msg);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned int getLastSeqNumberFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);
		static uint64_t getFIDLengthFromMessage(unsigned char* buffer);
		static unsigned char* getFIDFromMessage(unsigned char* buffer, uint64_t fIDLength);

	private:
		unsigned int clientId;
		unsigned int lastSeqNumber;
		unsigned char flags;
		uint64_t fIDLength;
		unsigned char* fID;
	};
}