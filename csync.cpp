#include <iostream>
using namespace std;


bool parseCommandLine(int argc, char* argv[],string& folderpath,string &hostname,bool& isServer,int& port)
{
for (int i = 1; i < argc; i++) {

    if (strcmp(argv[i],"-f")==0) { // folder path
        folderpath = argv[i+1]; //BUG: this gives index out of range , if used wrong.
    } else if (strcmp(argv[i],"-s")==0) { // server mode
        isServer=true;
    } else if (strcmp(argv[i],"-p")==0) { // port
        port = atoi(argv[i + 1]);
    } else if (strcmp(argv[i],"-h")==0) { // hostname
        hostname = argv[i+1];
    }
	

}
if (isServer){
	cout<<"running server mode"<<endl;
}
cout<<"running client mode"<<endl;
if (folderpath==""){
	cout<<"you must enter a folder path"<<endl;
	cout<<"aborting...";
	return false;
}
if (hostname==""){
	cout<<"you must enter a hostname"<<endl;
	cout<<"aborting";
	return false;
}
return true;
}
int main(int argc,char* argv[]){

string FOLDER_PATH="",HOST_NAME="";
bool isServer=false;
int PORT=3005; //default value


bool canParse=parseCommandLine(argc,argv,FOLDER_PATH,HOST_NAME,isServer,PORT);

if(!canParse)
{ 
	return 0;
}

cout<<"using port: "<<PORT<<endl;
cout<<"uploading folder: "<<FOLDER_PATH << endl;
cout<<"hostname: "<<HOST_NAME<<endl ;
}
