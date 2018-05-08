#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>

#pragma once

// At the moment the struct item is useless. Can later get used to save important information:
struct Folder {
    std::string path = "";
    bool isCreated = false;
    };

struct File {
	std::string name;
    bool isOpen = false;
    long unsigned int size = 0;
    std::ifstream fd;
    long last_part = 0;


};

class Filesystem {
private:
	std::string path;
	std::unordered_map <std::string, File*> files;
	std::list<Folder*> folders;
    Folder* genFolder(std::string path);
    File* genFile(std::string FID);

public:
	Filesystem(std::string p);
	static bool exists(std::string path);
	int genMap();
    int genMap(std::string path);
    long unsigned int filesize(const std::string FID);
    int readFile(std::string FID, char* buffer, int partNr, int length);
	std::string toString();
};