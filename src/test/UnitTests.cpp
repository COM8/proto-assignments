#include "UnitTests.h"

using namespace test;
using namespace sec;
using namespace net;

void UnitTests::runTests()
{
    cout << "Running test:\n";
    testDiffi();
    cout << "Success!\n";
}

void UnitTests::testDiffi()
{
    cout << "Started Diffi test\n";
    DiffieHellman clientDiffi = DiffieHellman();
    DiffieHellman serverDiffi = DiffieHellman();

    clientDiffi.clientStartConnection();
    cout << "Client prime: " << clientDiffi.getPrime() << " primitive root: " << clientDiffi.getPrimitiveRoot() << " client pub: " << clientDiffi.getPubKey() << '\n';
    serverDiffi.onServerReceive(clientDiffi.getPrime(), clientDiffi.getPrimitiveRoot(), clientDiffi.getPubKey());
    cout << "Server pub: " << serverDiffi.getPubKey() << '\n';
    clientDiffi.onClientReceive(serverDiffi.getPubKey());

    unsigned char *testData = new unsigned char[9]{};
    unsigned char *refData = new unsigned char[9]{};
    unsigned int testDataLength = 9;
    unsigned int refDataLength = 9;
    for (int i = 0; i < testDataLength; i++)
    {
        testData[i] = i;
        refData[i] = i;
    }

    clientDiffi.encrypt(testData, &testDataLength);
    serverDiffi.decrypt(testData, &testDataLength);

    if (testDataLength != refDataLength)
    {
        cout << "testDataLength != refDataLength\n";
        cout << testDataLength << " != " << refDataLength << '\n';
        cout << "Test data: \n";
        AbstractMessage::printByteArray(testData, testDataLength);
        cout << "Ref data: \n";
        AbstractMessage::printByteArray(refData, testDataLength);
        exit(-1);
    }

    comp(testData, refData, testDataLength);

    delete testData;
    delete refData;
    cout << "Started Diffi test\n";
}

void UnitTests::comp(unsigned char *result, unsigned char *refData, unsigned int testDataLength)
{
    for (int i = 0; i < testDataLength; i++)
    {
        if (result[i] != refData[i])
        {
            cout << "Comp failed at " << i << '\n';
            cout << "Result data: \n";
            AbstractMessage::printByteArray(result, testDataLength);
            cout << "Ref data: \n";
            AbstractMessage::printByteArray(refData, testDataLength);
            exit(-1);
        }
    }
}