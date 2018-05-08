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
    static bool exists(std::string path);
    };

class FilesystemClient: Filesystem {
private:
    std::string path;

public:
    std::unordered_map <std::string, File*> files;
    std::list<Folder*> folders;
    FilesystemClient(std::string p);
    static bool exists(std::string path);
    int genMap();
    int genMap(std::string path);
    int readFile(std::string FID, char* buffer, int partNr, int length);
    WorkingSet* getWorkingSet();
	std::string toString();
};

class FilesystemServer: Filesystem {

    };