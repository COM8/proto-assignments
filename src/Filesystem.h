#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>
#include <cstring>
#include <WorkingSet.h>
#include <lib/zedwood/md5.h>

#pragma once


struct ServerFile {
    char *hash = new char[32];
    unsigned int last_part = 0;
    };


class Filesystem {
protected:
    static Folder* genFolder(std::string path);
    static File* genFile(std::string FID);
public:
    const unsigned static int partLength = 900;
    static long unsigned int filesize(const std::string FID);
    static void calcSHA256(const std::string FID, char* buffer);
    static bool exists(std::string path);
    };

class FilesystemClient: Filesystem {
private:
    std::list<Folder*> folders;
    std::string path;
    bool isInFolders(std::string path);

public:
    FilesystemClient(std::string p);
    static bool exists(std::string path);
    std::unordered_map <std::string, File*> files;
    int genMap();
    int genMap(std::string path);
    int genMap(std::string path, std::unordered_map <std::string, File*> *files, std::list<Folder*> *folders, std::list<std::string> *deleteFile, std::list<std::string> *deleteFolder);
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
    std::unordered_map <std::string, ServerFile*> files;
    void createPath();
    void folderClean(std::string path);
    void fileClean(std::string file);
    void readFolderFile();
    void readFileFile();
    char* intToArray(unsigned int i);
    unsigned int charToInt(char* buffer);
    void saveFolderFile();
    void saveFileFile();
    ServerFile* genServerFile(char* hash, unsigned int partNr);
public:
    FilesystemServer(std::string path);
    void genFile(std::string FID, char* hash);
    void genFolder(std::string path);
    void delFolder(std::string path);
    void delFile(std::string FID);
    int writeFilePart(std::string FID, char* buffer, unsigned int partNr, unsigned int length);
    void clearDirecotry();
    void close();
    };