#pragma once

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdint>
#include <iostream>
#include <bitset>
#include "lib/hash-library/crc32.h"
#include "Consts.h"

namespace net
{

struct Message
{
	int bufferLength;
	unsigned char *buffer;
};

struct ReadMessage
{
	unsigned short msgType;
	unsigned char *buffer;
	unsigned int bufferLength;
	char senderIp[INET_ADDRSTRLEN];
};

class AbstractMessage
{
  public:
	virtual void createBuffer(struct Message *msg) { msg->bufferLength = -1; };

	AbstractMessage(unsigned char type);
	AbstractMessage() = default;
	~AbstractMessage();
	unsigned char getType();
	void addChecksum(struct Message *msg, unsigned int checkSumOffsetBits);
	static bool isChecksumValid(struct ReadMessage *msg, unsigned int checkSumOffsetBits);
	static void printMessage(struct Message *msg);
	static void printByteArray(unsigned char *c, int length);
	static void printByte(unsigned char c);

  protected:
	unsigned char type;

	// Source: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
	static unsigned int caclCRC32(unsigned char *buffer, int bufferLength);

	static unsigned int getUnsignedIntFromMessage(unsigned char *buffer, int bitOffset);
	static unsigned int getUnsignedShortFromMessage(unsigned char *buffer, int bitOffset);
	static uint64_t getUnsignedInt64FromMessage(unsigned char *buffer, int bitOffset);

	static unsigned char *getBytesWithOffset(unsigned char *buffer, int bitOffset, int bitLength);
	static unsigned char *getBytesWithOffset(unsigned char *buffer, int bitOffset, uint64_t bitLength);
	static unsigned char getByteWithOffset(unsigned char *buffer, int bitOffset);
	void setBufferValue(struct Message *msg, unsigned char *value, int valueLength, int bitOffset);
	static void setBufferValue(unsigned char *buffer, unsigned char *value, int valueLength, int bitOffset);
	void setByteWithOffset(struct Message *msg, unsigned char value, int bitOffset);
	static void setByteWithOffset(unsigned char *buffer, unsigned char value, int bitOffset);
	void setBufferUnsignedInt(struct Message *msg, unsigned int i, int bitOffset);
	static void setBufferUnsignedInt(unsigned char *, unsigned int i, int bitOffset);
	void setBufferUnsignedShort(struct Message *msg, unsigned short i, int bitOffset);
	static void setBufferUnsignedShort(unsigned char *, unsigned short i, int bitOffset);
	void setBufferUint64_t(struct Message *msg, uint64_t i, int bitOffset);
	static void setBufferUint64_t(unsigned char *, uint64_t i, int bitOffset);
};
}