#pragma once

#include <cstdint>
#include "net/AbstractMessage.h"

namespace net
{

class AuthRequestMessage : public AbstractMessage
{
  public:
    static const unsigned int CHECKSUM_OFFSET_BITS = 72;

    AuthRequestMessage(unsigned int clientId, unsigned int passwordLength, unsigned char *password, unsigned int seqNumber);
    virtual void createBuffer(struct Message* msg);
    static unsigned int getClientIdFromMessage(unsigned char *buffer);
    static unsigned int getPasswordLengthFromMessage(unsigned char *buffer);
    static unsigned char *getPasswordFromMessage(unsigned char *buffer, unsigned int passwordLength);
    static unsigned int getSeqNumberFromMessage(unsigned char* buffer);

  private:
    unsigned int clientId;
    unsigned int passwordLength;
    unsigned char *password;
    unsigned int seqNumber;
};
}