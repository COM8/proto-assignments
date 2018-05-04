#pragma once

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdint>

namespace net {

	struct Message
	{
		int bufferLength;
		char* buffer;
	};

	struct ReadMessage
	{
		unsigned short msgType;
		unsigned char* buffer;
		unsigned int bufferLength;
		char senderIp[INET_ADDRSTRLEN];
	};

	class AbstractMessage {
	public:
		virtual void createBuffer(struct Message* msg) { msg->bufferLength = -1; };

		AbstractMessage(char type);
		AbstractMessage() = default;
		char getType();
		void addChecksum(struct Message* msg, unsigned int checkSumOffsetBits);
		static bool isChecksumValid(struct ReadMessage* msg, unsigned int checkSumOffsetBits);

	protected:
		char type;

		static unsigned int getUnsignedIntFromMessage(unsigned char* buffer, int bitOffset);
		static unsigned int getUnsignedShortFromMessage(unsigned char* buffer, int bitOffset);
		static uint64_t getUnsignedInt64FromMessage(unsigned char* buffer, int bitOffset);

		static unsigned char* getBytesWithOffset(unsigned char* buffer, int bitOffset, int bitLength);
		static unsigned char* getBytesWithOffset(unsigned char* buffer, int bitOffset, uint64_t bitLength);
		void setBufferValue(struct Message* msg, char* value, int valueLength, int bitOffset);
		void setByteWithOffset(struct Message* msg, char value, int bitOffset);
		static unsigned char getByteWithOffset(unsigned char* buffer, int bitOffset);
	};
}