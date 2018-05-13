#include "net/ClientHelloMessage.h"

using namespace net;
using namespace std;

ClientHelloMessage::ClientHelloMessage(unsigned short port, unsigned int clientId, unsigned char flags) : AbstractMessage(1 << 4)
{ // 00010000
	this->port = port;
	this->clientId = clientId;
	this->flags = flags;
}

void ClientHelloMessage::createBuffer(struct Message *msg)
{
	msg->buffer = new unsigned char[11]{};
	msg->bufferLength = 11;

	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add port:
	setBufferUnsignedShort(msg, port, 8);

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 24);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned char ClientHelloMessage::getFlagsFromMessage(unsigned char *buffer)
{
	return buffer[0] & 0xF; // 4 bit offset
}

unsigned short ClientHelloMessage::getPortFromMessage(unsigned char *buffer)
{
	return getUnsignedShortFromMessage(buffer, 8);
}

unsigned int ClientHelloMessage::getClientIdFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 24);
}
