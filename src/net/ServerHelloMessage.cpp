#include "net/ServerHelloMessage.h"

using namespace net;
using namespace std;

ServerHelloMessage::ServerHelloMessage(unsigned short port, unsigned int clientId, unsigned int seqNumber, unsigned char flags, unsigned long pubKey) : AbstractMessage(2 << 4)
{
	this->port = port;
	this->clientId = clientId;
	this->seqNumber = seqNumber;
	this->flags = flags;
	this->pubKey = pubKey;
}

void ServerHelloMessage::createBuffer(struct Message *msg)
{
	msg->buffer = new unsigned char[19]{};
	msg->bufferLength = 19;

	// Add type:
	msg->buffer[0] |= type;

	// Add flags:
	msg->buffer[0] |= flags;

	// Add client id:
	setBufferUnsignedInt(msg, clientId, 8);

	// Add sequence number:
	setBufferUnsignedInt(msg, seqNumber, 40);

	// Add port:
	setBufferUnsignedShort(msg, port, 56);

	// Public key:
	setBufferUnsignedInt(msg, pubKey, 88);

	// Add checksum:
	addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned char ServerHelloMessage::getFlagsFromMessage(unsigned char *buffer)
{
	return buffer[0] & 0xF; // 4 bit offset
}

unsigned int ServerHelloMessage::getClientIdFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 8);
}

unsigned short ServerHelloMessage::getPortFromMessage(unsigned char *buffer)
{
	return getUnsignedShortFromMessage(buffer, 56);
}

unsigned long ServerHelloMessage::getPubKeyFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 88);
}

unsigned int ServerHelloMessage::getSeqNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 40);
}