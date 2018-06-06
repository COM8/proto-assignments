#include "DiffieHellman.h"
using namespace sec;
DiffieHellman::DiffieHellman(){
    this->isSecure=false;
   srand(time(NULL));
}

void DiffieHellman::clientStartConnection(){
    unsigned long max=100000000;
    vector<unsigned long> primes;
    primes=this->get_primes(max);

    
    unsigned long myPrime=primes[rand()%primes.size()];
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

void DiffieHellman::encrypt(unsigned char *& toEncrypt, unsigned int toEncryptLength){
    std::string output = string((char *)toEncrypt,toEncryptLength);
    //output=v_encrypt(output,this->key);
    std::string b64_str = base64_encode(output);
	output = encrypt_vigenere(b64_str, this->key);
	// std::cout << vigenere_msg << std::endl;
    toEncrypt=(unsigned char *)output.c_str();
}

void DiffieHellman::decrypt(unsigned char *& toDecrypt, unsigned int toDecryptLength){
    std::string output = string((char *)toDecrypt, toDecryptLength);
    //output=v_decrypt(output,this->key);

    std::string newKey = extend_key(output, key);
	std::string b64_encoded_str = decrypt_vigenere(output, newKey);
	output = base64_decode(b64_encoded_str);
	
    
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

vector<unsigned long> DiffieHellman::get_primes(unsigned long max){
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

 unsigned long DiffieHellman::power(long long int a, long long int b, long long int P){
      if (b == 1)
                return a;
        
            else
                return (((long long int)pow(a, b)) % P);
}
