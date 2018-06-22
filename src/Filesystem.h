#pragma once

#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>
#include <cstring>
#include "WorkingSet.h"
#include "lib/hash-library/md5.h"
#include "lib/hash-library/crc32.h"
#include "Logger.h"
#include "Consts.h"
#include "ClientsToDo.h"


struct ServerFile {
    std::shared_ptr<std::array<char,32>> hash = std::make_shared<std::array<char,32>>();
    unsigned int last_part = 0;
    ServerFile(unsigned int last_part) {
        this->last_part = last_part;
    }
    ServerFile(char* hash, unsigned int last_part) {
        this->last_part = last_part;
        strcpy(this->hash->data(), hash);
    }
    ServerFile() {
    }
    static std::unique_ptr<struct ServerFile> genPointer(unsigned int last_part) {
        return std::make_unique<struct ServerFile>(ServerFile(last_part));
    }
    static std::unique_ptr<struct ServerFile> genPointer(char* hash, unsigned int last_part) {
        return std::make_unique<struct ServerFile>(ServerFile(hash, last_part));
    }
    //static std::unique_ptr<struct ServerFile> genPointer() {
    //    return std::make_unique<struct ServerFile>();
    //}
    };


class Filesystem {
protected:
    static std::shared_ptr<File> genFile(std::string FID);
    char* intToArray(unsigned int i);
    unsigned int charToInt(char* buffer);
public:
    const unsigned static int partLength = MAX_CONTENT_LENGTH;
    static long unsigned int filesize(const std::string FID);
    static void calcSHA256(const std::string FID, std::shared_ptr<std::array<char,32>> buffer);
    static void calcSHA256(const std::string FID, char* buffer);
    static void calcCRC32(char* in, char* out);
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
    std::list<std::shared_ptr<Folder>> folders;
    std::string path;
    bool isInFolders(std::string path);
    void compareFiles(std::string FID, std::shared_ptr<File> f);
    void genCRC32(std::string FID, std::shared_ptr<File> f);
    void saveFilesystem();
    void openFilesystem();
    

public:
    FilesystemClient(std::string p);
    static bool exists(std::string path);
    std::unordered_map <std::string, std::shared_ptr<File>> files;
    int genMap();
    int genMap(std::string path);
    int genMap(std::string path, std::unordered_map <std::string, std::shared_ptr<File>> *files, std::list<std::shared_ptr<Folder>> *folders, std::list<std::string> *deleteFile, std::list<std::string> *deleteFolder);
    int readFile(std::string FID, char *buffer, unsigned int partNr, bool *isLastPart);
    int writeFilePart(std::string FID, char* buffer, unsigned int partNr, unsigned int length);
    void close();
    WorkingSet* getWorkingSet();
	std::string filesToString();
    std::string foldersToString();
};
/*
   1. init class
   2. change Files
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
public:
    FilesystemServer();
    void init(std::string path, ClientsToDo* clientsToDo);
    void genFile(std::string FID, char* hash);
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