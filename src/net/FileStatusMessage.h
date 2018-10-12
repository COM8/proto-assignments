#pragma once

#include "AbstractMessage.h"
#include <cstdint>

namespace net {
	
	class FileStatusMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 104;

		FileStatusMessage(unsigned int clientId, unsigned int seqNumber, unsigned int lastFIDPartNumber, unsigned char flags, uint64_t fIDLength, unsigned char* fID);
		FileStatusMessage(unsigned int clientId, unsigned int seqNumber, unsigned char flags, uint64_t fIDLength, unsigned char* fID);
		virtual void createBuffer(struct Message* msg);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned int getLastFIDPartNumberFromMessage(unsigned char* buffer);
		static unsigned char getFlagsFromMessage(unsigned char* buffer);
		static unsigned int getSeqNumberFromMessage(unsigned char* buffer);
		static uint64_t getFIDLengthFromMessage(unsigned char* buffer);
		static unsigned char* getFIDFromMessage(unsigned char* buffer, uint64_t fIDLength);

	private:
		unsigned int clientId;
		unsigned int lastFIDPartNumber;
		unsigned int seqNumber;
		unsigned char flags;
		uint64_t fIDLength;
		unsigned char* fID;
	};
}
