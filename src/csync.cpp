#include "csync.h"

struct arg* parseParameter(int argc, char* argv[]){
    struct arg* temp = new arg();
    for(int i = 1; i<argc; i++) {
        if(strcmp(argv[i],"-h") == 0){
            temp->type = client;
            if(argc > ++i) {
                temp->host = string(argv[++i]);
            }
        }else if(strcmp(argv[i],"-p") == 0){
            if(argc > ++i) {
                temp->port = atoi(argv[++i]);
            }
        }else if(strcmp(argv[i],"-f") == 0){
            if(argc > ++i) {
                temp->dir = string(argv[i]);
            }
        }else if(strcmp(argv[i],"-s") == 0){
            temp->type = server;
        }else if(strcmp(argv[i],"-p") == 0){
            if(argc > ++i) {
              temp->port = atoi(argv[++i]);
            }
        }
    }
    if(temp->port == 0) {
            cerr << "Please specify port useing 1234 instead" << endl;
            temp->port = 1234;
    }   
    if(temp->type == client) {
        if((temp->host) == ""){
            cerr << "No hostname or ip-addr given "<< endl << helpString;
            exit(-1);
        }else if((temp->dir) == ""){
            cerr << "No sync folder given using current folder" <<endl;
            temp->dir = "./";
        }if (!filesystem::exists(temp->dir)){
            cerr << "given path doesn't exist terminating" << endl;
            exit(-1);
        }
    }else if(!temp->type == server){
        cerr << "No mode given please specify it next time" << endl << helpString;
        exit(-1);
    }
    return temp;
}

void launchServer(unsigned int port){
    //todo
}

void launchClient(unsigned int port, string host, string dir) {
    //todo
    filesystem fi = filesystem(dir);
    fi.genMap();
    cout << fi.toString();
}


int main(int argc, char* argv[]) {
    struct arg* t = parseParameter(argc, argv);
    if(t->type == server) {
        launchServer(t->port);
    }else{
        launchClient(t->port, t->host, t->dir);
    }
    return 0;
}