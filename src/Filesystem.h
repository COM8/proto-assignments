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
    std::string hash;
    bool isOpen = false;
    long unsigned int size = 0;
    std::ifstream fd;
    long unsigned last_part = 0;
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
    static std::string calcSHA256(const std::string FID);
    static bool exists(std::string path);
    };

class FilesystemClient: Filesystem {
private:
    std::string path;
    std::unordered_map <std::string, File*> files;
    std::list<Folder*> folders;

public:
    FilesystemClient(std::string p);
    static bool exists(std::string path);
    int genMap();
    int genMap(std::string path);
    int readFile(std::string FID, char* buffer, int partNr, int length);
    WorkingSet* getWorkingSet();
	std::string toString();
};

class FilesystemServer: Filesystem {
private:
    std::string path = "";
public:
    FilesystemServer(std::string path);
    void genFolder(std::string path);
    void delFolder(std::string path);
    void delFile(std::string path);
    void writeFilePart(std::string FID, int part);
    void loadDirectory();
    };