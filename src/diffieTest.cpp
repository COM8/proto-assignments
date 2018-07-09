#include <iostream>
#include "sec/DiffieHellman.h"
using namespace std;
using namespace sec;

//g++ diffieTest.cpp sec/*.cpp -std=c++17
/*int main(){

    
    unsigned char * text=(unsigned char * )"I'm a client and I'm sending this text, this is the working demo !!";

 unsigned int *myLen=(unsigned int *)strlen((const char *)text);
    DiffieHellman client_DH=DiffieHellman();
    DiffieHellman server_DH=DiffieHellman();

    client_DH.clientStartConnection();
    std::cout<<client_DH.getPrime()<<std::endl;
    server_DH.onServerReceive(client_DH.getPrime(),client_DH.getPrimitiveRoot(),client_DH.getPubKey());

    client_DH.onClientReceive(server_DH.getPubKey());

    if (client_DH.isConnectionSecure() && server_DH.isConnectionSecure()){

       
        cout<<"Real text: "<<text<<endl;
        cout<<"Plain length:" <<reinterpret_cast<uintptr_t>(myLen)<<endl;
        client_DH.encrypt(text,myLen);
        cout<<"Encrypted by Client: "<<text<<endl;

        cout<<"Encrypted Length:"<<reinterpret_cast<uintptr_t>(myLen)<<endl;
        server_DH.decrypt(text,myLen);
        cout<<"Decrypted by Server: " <<text<<endl;
        cout<<"Decrypted Length:" <<reinterpret_cast<uintptr_t>(myLen)<<endl;
    }else{
        cout<<"Not secure"<<endl;
    }

    
    
}*/