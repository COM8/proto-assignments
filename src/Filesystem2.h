#pragma once


#include <map>
#include <list>
#include <string>
#include <memory>
#include <cstring>
#include <unordered_map>

#include "Consts.h"
#include "WorkingSet.h"
#include "ClientsToDo.h"
#include "Filestructures.h"
#include "lib/hash-library/md5.h"
#include "lib/hash-library/crc32.h"


class Filesystem {
public:
   static unsigned int filesysze(const std::string FID);
   static void calcSHA256(const std::string FID, char* buffer);
   static void calcCRC32(char* buffer, unsigned int bufferLength, char crc32Bytes[CRC32::HashBytes]);
   static bool exists(std::string path);
};


class FilesystemClient:Filesystem {
private:
   std::unordered_map <std::string, std::shared_ptr<File>> files;
   std::list<std::shared_ptr<Folder>> folders;
   std::string path;
   int genMap(const std::string path, std::unordered_map <std::string, std::shared_ptr<File>> *files, std::list<std::shared_ptr<Folder>> *folders, std::list<std::string> *deleteFile, std::list<std::string> *deleteFolder);
   void compareFiles(const std::string FID, File* f);
   void genCRC32(const std::string FID, File* f);
   void openFilesystem();
   void closeFilesystem();
   std::shared_ptr<File> genFileOBJ(const std::string);
public:
   FilesystemClient(const std::string path);
   void close();
   int readFilePart(const std::string FID, char* buffer, unsigned int partNr);
   int writeFilePart(const std::string FID, char* buffer, unsigned int partNr, unsigned int length);
   void genFile(const std::string FID, char* hash);
   void genFolder(const std::string path);
   void delFile(const std::string FID);
   void delFolder(const std::string path);
   WorkingSet* getWorkingSet();
   std::string filesToStirng();
   std::string folderToString();
};


class FilesystemServer:Filesystem {
private:
   std::string path = "";
   std::unordered_map <std::string, bool> folders;
   std::unordered_map <std::string, std::unique_ptr<ServerFile>> files;
   ClientsToDo* clientsToDo;
   bool createPath();
   void removeFolder(const std::string path);
   void removeFile(const std::string path);
   void openFilesystem();
   void closeFilesystem();
   void saveFolders();
   void saveFiles();
   void openFiles();
   void openFolders();
   void genFile(const std::string FID, char* hash);
   void removeUnknownFiles();
public:
   FilesystemServer(const std::string path, ClientsToDo* clientsToDo);
   void close();
   void genFile(const std::string FID, char* hash, unsigned int clientID);
   void genFolder(const std::string path, unsigned int clientID);
   void delFile(const std::string FID, unsigned int clientID);
   void delFolder(const std::string path, unsigned int clientID);
   unsigned int getLastPart(std::string FID);
   int readFilePart(const std::string FID, char* buffer, unsigned int partNr);
   int writeFilePart(const std::string FID, char* buffer, unsigned int partNr, unsigned int length, unsigned int clientID);
};
