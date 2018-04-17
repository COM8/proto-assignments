#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdint>
#include <iomanip>
#include <dirent.h>
#include <stdio.h>
using namespace std;

std::string hashfile(string filename) //FIXME: this is from stackoverflow.
{ 
  std::ifstream fp(filename.c_str());
  std::stringstream ss;
  
  // Unable to hash file, return an empty hash.
  if (!fp.is_open()) {
    return "";
  }
  
  // Hashing
  uint32_t magic = 5381;
  char c;
  while (fp.get(c)) {
    magic = ((magic << 5) + magic) + c; // magic * 33 + c
  }

  ss << std::hex << std::setw(8) << std::setfill('0') << magic;
  return ss.str();
}

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

void listFiles(string folderpath)
{
   DIR *dirp;
   dirp=opendir(folderpath.c_str());
   struct dirent *directory;
   if (dirp){
       while ((directory = readdir(dirp)) != NULL)
        {
            cout<<directory->d_name<<" --> "<<hashfile(folderpath+"/"+directory->d_name)<<endl;
            
        }

        closedir(dirp);
   }
}