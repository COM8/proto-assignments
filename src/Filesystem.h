#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>

#pragma once

struct Folder {
    std::string path = "";
    bool isCreated = false;
    };

struct File {
	std::string name;
    char* hash = new char[32];
    bool isOpen = false;
    unsigned int size = 0;
    std::ifstream fd;
    unsigned last_part = 0;
};

struct ServerFile {
    char* hash = new char[32];
    unsigned int last_part = 0;
    };

struct WorkingSet {
    std::list<Folder*> *folders;
    std::unordered_map <std::string, File*> *files;
    };

class Filesystem {
protected:
    static Folder* genFolder(std::string path);
    static File* genFile(std::string FID);
public:
    static long unsigned int filesize(const std::string FID);
    static void calcSHA256(const std::string FID, char* buffer);
    static bool exists(std::string path);
    };

class FilesystemClient: Filesystem {
private:
    std::list<Folder*> folders;
    std::string path;

public:
    FilesystemClient(std::string p);
    static bool exists(std::string path);
    std::unordered_map <std::string, File*> files;
    int genMap();
    int genMap(std::string path);
    int readFile(std::string FID, char* buffer, int partNr, int length);
    void close();
    WorkingSet* getWorkingSet();
	std::string toString();
};

class FilesystemServer: Filesystem {
private:
    std::string path = "";
    std::unordered_map <std::string, bool> folders;
    std::unordered_map <std::string, ServerFile*> files;
    void createPath();
    void folderClean(std::string path);
    void fileClean(std::string file);
    ServerFile* genServerFile(char* hash);
public:
    FilesystemServer(std::string path);
    void genFile(std::string FID, char* hash);
    void genFolder(std::string path);
    void delFolder(std::string path);
    void delFile(std::string FID);
    int writeFilePart(std::string FID, char* buffer, int partNr, int length);
    void clearDirecotry();
    void close();
    };