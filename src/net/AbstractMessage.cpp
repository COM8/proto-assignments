#include "net/AbstractMessage.h"

using namespace net;

AbstractMessage::AbstractMessage(char type) {
	this->type = type;
}

char AbstractMessage::getType() {
	return type;
}

void calcChecksum(char& buffer, int& bufferLength, unsigned int startIndex) {
	// ToDo: Calc checksum and add it
}