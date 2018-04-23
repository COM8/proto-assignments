#include "net/ClientHelloMessage.h"

using namespace net;

ClientHelloMessage::ClientHelloMessage(unsigned int port) : AbstractMessage(1 << 4) { // 00010000
	this->port = port;
}

void ClientHelloMessage::createBuffer(struct Message* msg) {
	msg->buffer = new char[5];
	msg->buffer[0] = msg->buffer[0] | 1;
	msg->bufferLength = 5;
}