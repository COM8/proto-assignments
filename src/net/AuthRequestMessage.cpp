#include "AuthRequestMessage.h"

using namespace net;

AuthRequestMessage::AuthRequestMessage(unsigned int clientId, unsigned int passwordLength, unsigned char *password, unsigned int seqNumber) : AbstractMessage(8 << 4)
{
    this->clientId = clientId;
    this->passwordLength = passwordLength;
    this->password = password;
    this->seqNumber = seqNumber;
}

void AuthRequestMessage::createBuffer(struct Message *msg)
{
    msg->buffer = new unsigned char[17+passwordLength]{};
    msg->bufferLength = 17+passwordLength;

    // Add type:
    msg->buffer[0] |= type;

    // Add client id:
    setBufferUnsignedInt(msg, clientId, 8);

    // Add Sequence number:
    setBufferUnsignedInt(msg, seqNumber, 40);

    // Add Password length:
    setBufferUnsignedInt(msg, passwordLength, 104);

    // Add Password:
    setBufferValue(msg, password, passwordLength, 136);

    // Add checksum:
    addChecksum(msg, CHECKSUM_OFFSET_BITS);
}

unsigned int AuthRequestMessage::getClientIdFromMessage(unsigned char *buffer)
{
    return getUnsignedIntFromMessage(buffer, 8);
}

unsigned int AuthRequestMessage::getPasswordLengthFromMessage(unsigned char *buffer)
{
    return getUnsignedIntFromMessage(buffer, 104);
}

unsigned char *AuthRequestMessage::getPasswordFromMessage(unsigned char *buffer, unsigned int passwordLength)
{
    return getBytesWithOffset(buffer, 136, (uint64_t)passwordLength);
}

unsigned int AuthRequestMessage::getSeqNumberFromMessage(unsigned char *buffer)
{
	return getUnsignedIntFromMessage(buffer, 40);
}