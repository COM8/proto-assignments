#include "AuthRequestMessage.h"

using namespace net;

AuthRequestMessage::AuthRequestMessage(unsigned int clientId, unsigned int passwordLength, unsigned char *password) : AbstractMessage(8 << 4)
{
    this->clientId = clientId;
    this->passwordLength = passwordLength;
    this->password = password;
}

void AuthRequestMessage::createBuffer(struct Message *msg)
{
    msg->buffer = new unsigned char[9]{};
    msg->bufferLength = 9;

    // Add type:
    msg->buffer[0] |= type;

    // Add client id:
    setBufferUnsignedInt(msg, clientId, 8);

    // Password length:
	setBufferUnsignedInt(msg, passwordLength, 184);

	// Password:
	setBufferValue(msg, password, passwordLength, 216);

    // Add checksum:
    addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int AuthRequestMessage::getClientIdFromMessage(unsigned char *buffer)
{
    return getUnsignedIntFromMessage(buffer, 8);
}

unsigned int AuthRequestMessage::getPasswordLengthFromMessage(unsigned char *buffer)
{
    return getUnsignedIntFromMessage(buffer, 72);
}

unsigned char *AuthRequestMessage::getPasswordFromMessage(unsigned char *buffer, unsigned int passwordLength)
{
    return getBytesWithOffset(buffer, 104, (uint64_t)passwordLength);
}