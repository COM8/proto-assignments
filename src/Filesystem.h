#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>
#include <cstring>
#include <WorkingSet.h>
#include <lib/hash-library/md5.h>

#pragma once

#define HASHPARTSIZE 16777216
#define PARTLENGTH 900


struct ServerFile {
    char *hash = new char[32];
    unsigned int last_part = 0;
    ServerFile(char *hash, unsigned int last_part) {
        this->hash = hash;
        this->last_part = last_part;
    }
    ServerFile() {
    }
    static std::unique_ptr<struct ServerFile> genPointer(char *hash, unsigned int last_part) {
        return std::make_unique<struct ServerFile>(ServerFile(hash, last_part));
    }  
    };


class Filesystem {
protected:
    static std::shared_ptr<File> genFile(std::string FID);
public:
    const unsigned static int partLength = PARTLENGTH;
    static long unsigned int filesize(const std::string FID);
    static void calcSHA256(const std::string FID, char* buffer);
    static bool exists(std::string path);
    };

class FilesystemClient: Filesystem {
private:
    std::list<std::shared_ptr<Folder>> folders;
    std::string path;
    bool isInFolders(std::string path);

public:
    FilesystemClient(std::string p);
    static bool exists(std::string path);
    std::unordered_map <std::string, std::shared_ptr<File>> files;
    int genMap();
    int genMap(std::string path);
    int genMap(std::string path, std::unordered_map <std::string, std::shared_ptr<File>> *files, std::list<std::shared_ptr<Folder>> *folders, std::list<std::string> *deleteFile, std::list<std::string> *deleteFolder);
    int readFile(std::string FID, char *buffer, unsigned int partNr, bool *isLastPart);
    void close();
    WorkingSet* getWorkingSet();
	std::string filesToString();
    std::string foldersToString();
};

class FilesystemServer: Filesystem {
private:
    std::string path = "";
    std::unordered_map <std::string, bool> folders;
    std::unordered_map <std::string, std::unique_ptr<ServerFile>> files;
    void createPath();
    void folderClean(std::string path);
    void fileClean(std::string file);
    void readFolderFile();
    void readFileFile();
    char* intToArray(unsigned int i);
    unsigned int charToInt(char* buffer);
    void saveFolderFile();
    void saveFileFile();
public:
    FilesystemServer(std::string path);
    void genFile(std::string FID, char* hash);
    void genFolder(std::string path);
    void delFolder(std::string path);
    void delFile(std::string FID);
    unsigned int getLastPart(std::string FID);
    int writeFilePart(std::string FID, char* buffer, unsigned int partNr, unsigned int length);
    void clearDirecotry();
    void close();
    };