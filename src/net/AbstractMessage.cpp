#include "net/AbstractMessage.h"

using namespace net;
using namespace std;

AbstractMessage::AbstractMessage(char type) {
	this->type = type;
}

char AbstractMessage::getType() {
	return type;
}

void AbstractMessage::addChecksum(struct Message* msg, int bitOffset) {
	// ToDo: Calc checksum and add it
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