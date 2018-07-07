#pragma once

#include <string>
#include<math.h>
#include <vector>
#include <sstream>
#include <cstring>
#include "b64.h"
#include "vigenere.h"
using namespace std;
namespace sec{
class DiffieHellman{
    public:
        DiffieHellman();
        void clientStartConnection();
        void onClientReceive(unsigned long otherPub);
        void onServerReceive(unsigned long P,unsigned long G,unsigned long otherPub);
        bool isConnectionSecure();

        void encrypt(unsigned char *& toEncrypt, unsigned int *toEncryptLength);
        void decrypt(unsigned char *& toDecrypt, unsigned int *toDecryptLength);

        

        unsigned long getPrime();
        unsigned long getPrimitiveRoot();
        unsigned long getPubKey();
        
    private:
        std::string key;
        bool isSecure;
        unsigned long otherPub,sharedKey,mySecret;
        unsigned long P,G,myPub;

        vector<unsigned long> get_primes(unsigned long max);

        unsigned long power(long long int a, long long int b, long long int P);

};

}