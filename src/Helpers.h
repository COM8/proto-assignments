#pragma once

#include <iostream>
#include <bitset>
#include "net/AbstractMessage.h"

void printMessage(struct net::Message* msg);
void printByteArray(const char* c, int length);
void printByte(char c);