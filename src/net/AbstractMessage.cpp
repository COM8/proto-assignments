#include "net/AbstractMessage.h"

using namespace net;

AbstractMessage::AbstractMessage(char type) {
	this->type = type;
}

char AbstractMessage::getType() {
	return type;
}