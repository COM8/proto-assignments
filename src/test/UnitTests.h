#pragma once

#include <iostream>
#include "sec/DiffieHellman.h"
#include "net/AbstractMessage.h"

namespace test
{
class UnitTests
{
public:
  void runTests();

private:
  void testDiffi();
  void comp(unsigned char *result, unsigned char *refData, unsigned int testDataLength);
};
}