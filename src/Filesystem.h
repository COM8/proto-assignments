#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>

#pragma once

// At the moment the struct item is useless. Can later get used to save important information:
struct File {
	std::string name;
    long unsigned int size = 0;

};

class Filesystem {
private:
	std::string path;
	std::unordered_map <std::string, File*> files;
	std::list<std::string> folders;

public:
	Filesystem(std::string p);
	static bool exists(std::string path);
	int genMap();
    int genMap(std::string path);
    long unsigned int filesize(const std::string filename);
	std::string toString();
};