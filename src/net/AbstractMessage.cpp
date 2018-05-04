#include "net/AbstractMessage.h"

using namespace net;
using namespace std;

AbstractMessage::AbstractMessage(char type) {
	this->type = type;
}

char AbstractMessage::getType() {
	return type;
}

void AbstractMessage::addChecksum(struct Message* msg, unsigned int checkSumOffsetBits) {
	// ToDo: Calc checksum and add it
}

bool AbstractMessage::isChecksumValid(struct ReadMessage* msg, unsigned int checkSumOffsetBits) {
	return true;
}

void AbstractMessage::setBufferValue(struct Message* msg, char* value, int valueLength, int bitOffset) {
	for (int i = 0; i < valueLength; ++i)
	{
		setByteWithOffset(msg, value[i], bitOffset + (8*i));
	}
}

void AbstractMessage::setByteWithOffset(struct Message* msg, char value, int bitOffset) {
	int byteIndex = bitOffset/8;

	if (bitOffset % 8 == 0) {
		msg->buffer[byteIndex] = value;
	}
	else {
		char first = (unsigned char)value >> (bitOffset % 8);
		char second = (unsigned char)value << (bitOffset % 8);
		msg->buffer[byteIndex++] |= first;
		msg->buffer[byteIndex] |= second;
	}
}

unsigned char* AbstractMessage::getBytesWithOffset(unsigned char* buffer, int bitOffset, int bitLength) {
	return getBytesWithOffset(buffer, bitOffset, (uint64_t)bitOffset);
}

unsigned char* AbstractMessage::getBytesWithOffset(unsigned char* buffer, int bitOffset, uint64_t bitLength) {
	uint64_t lengthBytes = bitLength/8;
	unsigned char* result = new unsigned char[lengthBytes];
	for(uint64_t i = 0; i < lengthBytes; i++) {	
		result[i] = getByteWithOffset(buffer, bitOffset + i*8);
	}

	int lengthMod = bitLength % 8;
	if(lengthMod != 0) {
		result[lengthBytes-1] &= ((char)-1) >> (lengthMod -1); 
	}

	return result;
}

unsigned char AbstractMessage::getByteWithOffset(unsigned char* buffer, int bitOffset) {
	int bitOffsetMod = bitOffset % 8;
	int byteOffset = bitOffset / 8;
	if(bitOffsetMod == 0) {
		return buffer[byteOffset];
	}
	else {
		return (buffer[byteOffset] << bitOffsetMod) | ((unsigned char)(buffer[byteOffset+1]) >> bitOffsetMod);
	}
}

unsigned int AbstractMessage::getUnsignedIntFromMessage(unsigned char* buffer, int bitOffset) {
	unsigned char* intArray = AbstractMessage::getBytesWithOffset(buffer, bitOffset, 32);
	return static_cast<int>(intArray[0]) << 24 | intArray[1] << 16 | intArray[2] << 8 | intArray[3];
}

unsigned int AbstractMessage::getUnsignedShortFromMessage(unsigned char* buffer, int bitOffset) {
	unsigned char* shortArray = AbstractMessage::getBytesWithOffset(buffer, bitOffset, 32);
	return static_cast<short>(shortArray[0]) << 8 | shortArray[1];
}

uint64_t AbstractMessage::getUnsignedInt64FromMessage(unsigned char* buffer, int bitOffset) {
	unsigned char* int64Array = AbstractMessage::getBytesWithOffset(buffer, bitOffset, 64);
	return static_cast<uint64_t>(int64Array[0]) << 56 | int64Array[1] << 48 | int64Array[2] << 40 
						| int64Array[3] << 32 | int64Array[4] << 24 | int64Array[5] << 16 
						| int64Array[6] << 8 | int64Array[7];
}