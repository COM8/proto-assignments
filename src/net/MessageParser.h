#pragma once

#include <iostream>
#include "net/AbstractMessage.h"
#include "net/ClientHelloMessage.h"

void parseMessage(unsigned char readByte, int socketFD, net::AbstractMessage* result);