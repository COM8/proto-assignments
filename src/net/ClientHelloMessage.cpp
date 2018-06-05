#include "net/ClientHelloMessage.h"

using namespace net;
using namespace std;

ClientHelloMessage::ClientHelloMessage(unsigned short port, unsigned int clientId, unsigned char flags, unsigned long prime, unsigned long primRoot, unsigned long pubKey) : AbstractMessage(1 << 4)
{
	this->port = port;
	this->clientId = clientId;
	this->flags = flags;
	this->prime = prime;
	this->primRoot = primRoot;
	this->pubKey = pubKey;
}

void ClientHelloMessage::createBuffer(struct Message *msg)
{
	msg->buffer = new unsigned char[123]{};
	msg->bufferLength = 123;

	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add port:
	setBufferUnsignedShort(msg, port, 8);

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 24);

	// Prime number:
	setBufferUnsignedInt(msg, prime, 56);

	// Primitive root:
	setBufferUnsignedInt(msg, primRoot, 88);

	// Public key:
	setBufferUnsignedInt(msg, pubKey, 120);

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

unsigned long ClientHelloMessage::getPubKeyFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 120);
}

unsigned long ClientHelloMessage::getPrimeNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 56);
}

unsigned long ClientHelloMessage::getPrimitiveRootFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 88);
}
