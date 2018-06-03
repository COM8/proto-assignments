#include "DiffieHellman.h"


DiffieHellman::DiffieHellman(){
    this->isSecure=false;
}

void DiffieHellman::ClientStartConnection(){
    int max=10000000;
    vector<unsigned long> primes;
    primes=this->get_primes(max);
    unsigned long myPrime=primes[rand()%max];
    this->P=myPrime;          

    G=9;
    this->mySecret=4;

    this->myPub=power(this->G,this->mySecret,this->P);

    //SEND TO SERVER
    P,G,myPub;
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

    this->mySecret=3;
    this->myPub=power(this->G,this->mySecret,this->P);

    this->sharedKey=power(this->otherPub,this->mySecret,this->P);
    std::stringstream stream;
    stream<<std::hex<<this->sharedKey;

    this->key=stream.str();

    //send to client
    this->myPub;

    
    this->isSecure=true;
}

bool DiffieHellman::isConnectionSecure(){
    return this->isSecure;
}

void DiffieHellman::Encrypt(unsigned char * toEncrypt){
    std::string output = (string)(char *)toEncrypt;
    for (int i = 0; i < output.size(); i++){
        char k = toEncrypt[i] ^ this->key[i % (sizeof(key) / sizeof(char))];
       
        output[i]=k;
}
toEncrypt=(unsigned char *)output.c_str();
}

void DiffieHellman::Decrypt(unsigned char * toDecrypt){
    std::string output = (string)(char *)toDecrypt;
           
    for (int i = 0; i < output.size(); i++){
        char k = toDecrypt[i] ^ this->key[i % (sizeof(key) / sizeof(char))];
       
        output[i]=k;
    }
    
   toDecrypt=(unsigned char *)output.c_str();
}