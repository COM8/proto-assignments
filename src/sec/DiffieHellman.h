#pragma once

#include <string>
#include<math.h>
#include <vector>
#include <sstream>
#include <cstring>
using namespace std;
namespace sec{
class DiffieHellman{
    public:
        DiffieHellman();
        void ClientStartConnection();
        void onClientReceive(unsigned long otherPub);
        void onServerReceive(unsigned long P,unsigned long G,unsigned long otherPub);
        bool isConnectionSecure();

        void Encrypt(unsigned char * toEncryp, unsigned int toEncryptLengtht);
        void Decrypt(unsigned char * toDecrypt, unsigned int toDecryptLengtht);

        

        unsigned long getPrime();
        unsigned long getPrimitiveRoot();
        unsigned long getPubKey();
        
    private:
        std::string key;
        bool isSecure;
        unsigned long otherPub,sharedKey,mySecret;
        unsigned long P,G,myPub;

        vector<unsigned long> get_primes(unsigned long max){
            vector<unsigned long> primes;
            char *sieve;
            sieve = new char[max/8+1];
            // Fill sieve with 1  
            memset(sieve, 0xFF, (max/8+1) * sizeof(char));
            for(unsigned long x = 2; x <= max; x++)
                if(sieve[x/8] & (0x01 << (x % 8))){
                    primes.push_back(x);
                    // Is prime. Mark multiplicates.
                    for(unsigned long j = 2*x; j <= max; j += x)
                        sieve[j/8] &= ~(0x01 << (j % 8));
                }
            delete[] sieve;
            return primes;
        }

        unsigned long power(long long int a, long long int b, long long int P){ 
            if (b == 1)
                return a;
        
            else
                return (((long long int)pow(a, b)) % P);
        }
        

};

}