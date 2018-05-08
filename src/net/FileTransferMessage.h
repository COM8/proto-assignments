#pragma once

#include "net/AbstractMessage.h"
#include <cstdint>

namespace net {
	
	class FileTransferMessage : public AbstractMessage {
	public:
		static const unsigned int CHECKSUM_OFFSET_BITS = 328;

		FileTransferMessage(unsigned int clientId, unsigned int seqNumber, unsigned char fileType, unsigned char* fileHash, uint64_t contentLength, unsigned char* content);
		void createBuffer(struct Message* msg);
		static unsigned int getClientIdFromMessage(unsigned char* buffer);
		static unsigned int getSeqNumberFromMessage(unsigned char* buffer);
		static unsigned char getFileTypeFromMessage(unsigned char* buffer);
		static unsigned char* getFileHashFromMessage(unsigned char* buffer);
		static uint64_t getContentLengthFromMessage(unsigned char* buffer);
		static unsigned char* getContentFromMessage(unsigned char* buffer, uint64_t fIDLength);

	private:
		unsigned int clientId;
		unsigned int seqNumber;
		unsigned char fileType;
		unsigned char* fileHash;
		uint64_t contentLength;
		unsigned char* content;
	};
}