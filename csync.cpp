#include <iostream>
#include "helpers.cpp"
using namespace std;



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

cout<<endl<<endl;

listFiles(FOLDER_PATH.c_str());
}
