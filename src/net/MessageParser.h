#pragma once

#include <iostream>
#include "net/AbstractMessage.h"
#include "net/ClientHelloMessage.h"

net::AbstractMessage* parseMessage(unsigned char readByte, int socketFD);