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