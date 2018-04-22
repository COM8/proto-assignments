#include <experimental/filesystem>
#include <iostream>
#include <string>
#pragma once
#include <unordered_map>
#include <list>

// At the moment the struct item is useless. Can later get used to save important information:
struct File {
	std::string name;
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
	std::string toString();
};