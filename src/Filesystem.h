#pragma once

#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>
#include <cstring>
#include "WorkingSet.h"
#include "net/AbstractMessage.h"
#include "lib/hash-library/md5.h"
#include "lib/hash-library/crc32.h"
#include "Logger.h"
#include "Consts.h"
#include "ClientsToDo.h"


class Filesystem {
protected:
    char* intToArray(unsigned int i);
    unsigned int charToInt(char* buffer);
public:
    const unsigned static int partLength = MAX_CONTENT_LENGTH;
    static long unsigned int filesize(const std::string FID);
    static void calcSHA256(const std::string FID, std::shared_ptr<std::array<char,32>> buffer);
    static void calcSHA256(const std::string FID, char* buffer);
    static void calcCRC32(char* buffer, size_t bufferLength, char crc32Bytes[CRC32::HashBytes]);
    static bool exists(std::string path);
    };

/*
    1. init class
    2. getWorkingset()
    3. sendChanges of working set
    4. writeChanges
    5. goto 2.
*/

class FilesystemClient: Filesystem {
private:
    std::unordered_map <std::string, std::shared_ptr<File>> files;
    std::list<std::shared_ptr<Folder>> folders;
    std::string path;
    bool isInFolders(std::string path);
    int genMap();
    int genMap(std::string path);
    int genMap(const std::string path, std::unordered_map <std::string, std::shared_ptr<File>> *files, std::list<std::shared_ptr<Folder>> *folders, std::list<std::string> *deleteFile, std::list<std::string> *deleteFolder);
    void compareFiles(const std::string FID, std::shared_ptr<File> f);
    void genCRC32(std::string FID, std::shared_ptr<File> f);
    void saveFilesystem();
    void openFilesystem();
    std::shared_ptr<File> genFileOBJ(const std::string FID);
public:
    FilesystemClient(std::string p);
    int readFile(const std::string FID, char *buffer, unsigned int partNr);
    int writeFilePart(const std::string FID, char* buffer, unsigned int partNr, unsigned int length);
    void close();
    void genFile(const std::string FID, char* hash);
    void genFolder(const std::string path);
    void delFolder(const std::string path);
    void delFile(const std::string FID);
    long unsigned int filesize(const std::string FID);
    WorkingSet* getWorkingSet();
	std::string filesToString();
    std::string foldersToString();

};

/*
   1. init class
   2. change Files@
   3. getWorkingset
   4. sync changes across clients
   5. goto 2
*/

class FilesystemServer: Filesystem {
private:
    std::string path = "";
    std::unordered_map <std::string, bool> folders;
    std::unordered_map <std::string, std::unique_ptr<ServerFile>> files;
    ClientsToDo* clientsToDo;
    bool createPath();
    void folderClean(std::string path);
    void fileClean(std::string file);
    void readFolderFile();
    void readFileFile();
    void saveFolderFile();
    void saveFileFile();
    void genFile(std::string FID, char* hash);
public:
    FilesystemServer();
    void init(std::string path, ClientsToDo* clientsToDo);
    void genFile(std::string FID, char* hash, unsigned int clientID);
    void genFolder(std::string path, unsigned int clientID);
    void delFolder(std::string path, unsigned int clientID);
    void delFile(std::string FID, unsigned int clientID);
    unsigned int getLastPart(std::string FID);
    int writeFilePart(std::string FID, char* buffer, unsigned int partNr, unsigned int length, unsigned int clientID);
    void clearDirecotry();
    WorkingSet* getWorkingSet();
    int readFile(std::string FID, char* buffer, unsigned int partNr);
    void close();
    };
