#include "DiffieHellman.h"
#include "encrypt.h"
#include "time.h"
using namespace sec;
DiffieHellman::DiffieHellman(){
    this->isSecure=false;
}

void DiffieHellman::ClientStartConnection(){
    unsigned long max=100000000;
    vector<unsigned long> primes;
    primes=this->get_primes(max);

    srand(5);
    unsigned long myPrime=primes[rand()%max];
    this->P=myPrime;          

    G=2;

    while ((power(G,((P-1)/2),P) == 1))
     {
          G+=1;
     }
    this->mySecret=rand()%15;

    this->myPub=power(this->G,this->mySecret,this->P);

    //SEND TO SERVER
    //--> P,G,myPub;
}

void DiffieHellman::onClientReceive(unsigned long otherPub){
    this->otherPub=otherPub;
    this->sharedKey=power(this->otherPub,this->mySecret,this->P);

    std::stringstream stream;
    stream<<std::hex<<this->sharedKey;

    this->key=stream.str();

    //send ACK 
    this->isSecure=true;
}

void DiffieHellman::onServerReceive(unsigned long P,unsigned long G,unsigned long otherPub){
     this->P=P;
    this->G=G;
    this->otherPub=otherPub;
    isSecure=false;

    this->mySecret=rand()%15;
    this->myPub=power(this->G,this->mySecret,this->P);

    this->sharedKey=power(this->otherPub,this->mySecret,this->P);
    std::stringstream stream;
    stream<<std::hex<<this->sharedKey;

    this->key=stream.str();

    //send to client
    //--->this->myPub;

    
    this->isSecure=true;
}

bool DiffieHellman::isConnectionSecure(){
    return this->isSecure;
}

void DiffieHellman::Encrypt(unsigned char * toEncrypt, unsigned int toEncryptLength){
    std::string output = string((char *)toEncrypt, toEncryptLength);
    output=encrypt(output,this->key);
    toEncrypt=(unsigned char *)output.c_str();
}

void DiffieHellman::Decrypt(unsigned char * toDecrypt, unsigned int toDecryptLengtht){
    std::string output = string((char *)toDecrypt, toDecryptLengtht);
    output=decrypt(output,this->key);
    
   toDecrypt=(unsigned char *)output.c_str();
}

unsigned long DiffieHellman::getPrime(){
    return this->P;
}

unsigned long DiffieHellman::getPrimitiveRoot(){
    return this->G;
}


unsigned long DiffieHellman::getPubKey(){
    return this->myPub;
}