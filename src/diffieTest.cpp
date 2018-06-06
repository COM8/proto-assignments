#include <iostream>
#include "sec/DiffieHellman.h"
using namespace std;
using namespace sec;
/*int main(){

    
    unsigned char * text=(unsigned char * )"I'm a client and I'm sending this text";

    DiffieHellman client_DH=DiffieHellman();
    DiffieHellman server_DH=DiffieHellman();

    client_DH.clientStartConnection();
    std::cout<<client_DH.getPrime()<<std::endl;
    server_DH.onServerReceive(client_DH.getPrime(),client_DH.getPrimitiveRoot(),client_DH.getPubKey());

    client_DH.onClientReceive(server_DH.getPubKey());

    if (client_DH.isConnectionSecure() && server_DH.isConnectionSecure()){

        cout<<"Real text: "<<text<<endl;

        client_DH.encrypt(text,strlen((const char *)text));
        cout<<"Encrypted by Client: "<<text<<endl;

        server_DH.decrypt(text,strlen((const char *)text));
        cout<<"Decrypted by Server: " <<text<<endl;
    }else{
        cout<<"Not secure"<<endl;
    }

    
    
}*/