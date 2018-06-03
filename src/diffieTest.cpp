#include <iostream>
#include "DiffieHellman.h"
using namespace std;
int main(){

    
    unsigned char * text=(unsigned char * )"I'm a client and I'm sending this text";
    

    DiffieHellman client_DH=DiffieHellman();
    DiffieHellman server_DH=DiffieHellman();

    client_DH.ClientStartConnection();

    server_DH.onServerReceive(client_DH.getPrime(),client_DH.getPrimitiveRoot(),client_DH.getPubKey());

    client_DH.onClientReceive(server_DH.getPubKey());

    if (client_DH.isConnectionSecure() && server_DH.isConnectionSecure()){

        cout<<"Real text: "<<text<<endl;

        client_DH.Encrypt(text);
        cout<<"Encrypted by Client: "<<text<<endl;

        server_DH.Decrypt(text);
        cout<<"Decrypted by Server: " <<text<<endl;
    }else{
        cout<<"Not secure"<<endl;
    }

    
    
}