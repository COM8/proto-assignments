#pragma once

#include <list>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

#include "Consts.h"

struct NextPart{
	std::list<std::pair<unsigned int, unsigned int>> content;

	NextPart(){};

	NextPart(unsigned int partNr) {
		this->addPart(partNr);
	}

	bool isEmpty() {
		return this->content.empty();
	}

	int getNextPart() {
		if(!this->isEmpty()){
			return this->content.begin()->first;
		}
		return -1;
	}

	void addHoleFile(unsigned int length) {
		this->content.clear();
		this->content.push_back(std::pair<unsigned int, unsigned int> (0, length));
	}
	void addPart(unsigned int partNr) {
		if(!this->isEmpty()){
			for(std::list<std::pair<unsigned int, unsigned int>>::iterator i = content.begin(); i != content.end(); ++i){
				if(i->first <= partNr && partNr<= i->second) {
					return;
				}
				if(partNr < i->first) {
					if(partNr + 1 == i->first) {
						i->first = partNr;
						return;
					}
					else {
						this->content.insert(i, std::pair<unsigned int, unsigned int>(partNr, partNr));
						return;
					}
				}else {
					std::list<std::pair<unsigned int, unsigned int>>::iterator next = ++i;
					--i;    
					if(next->first <= partNr && partNr <= next->second) {
						return;
					}else {
						if(i->second < partNr && partNr < next->first) {
							bool c = false;
							if(i->second+1== partNr) {
								i->second = partNr;
								c = true;
							}else {
								if(next->first -1 == partNr) {
									next->first = partNr;
									c = true;
								}
								else {
									this->content.insert(next,std::pair<unsigned int, unsigned int>(partNr, partNr));
									return;
								}
							}
							if(c) {
								if(i->second == next->first || i->second +1 == next->first) {
									this->content.insert(i, std::pair<unsigned int, unsigned int>(i->first, next->second));
									this->content.erase(i);
									this->content.erase(next);
									return;
								}
								return;
								}
							}
						}
				}
			}
			std::list<std::pair<unsigned int, unsigned int>>::iterator i = content.begin();
			if(i->second + 1 == partNr) {
				i->second = partNr;
				//this->content.push_back(std::pair<unsigned int, unsigned int>(partNr, partNr));
				return;
			}else {
				this->content.push_back(std::pair<unsigned int, unsigned int>(partNr, partNr));
				return;
			}
		}
		else {
			this->content.push_back(std::pair<unsigned int, unsigned int>(partNr, partNr));
			return;
		}
	}

	int acknowledgePart(unsigned int partNr) {
		if(this->isEmpty()) {
			return -1;
		}
		std::list<std::pair<unsigned int, unsigned int>>::iterator i = this->content.begin();
		if(i->first == partNr) {
			if(i->first == i->second) {
				this->content.erase(i);
			}else {
				i->first = i->first +1;
			}
			return 0;
		}
		return -1;
	}

	void printNexPart() {
		for(std::pair<unsigned int, unsigned int> t: this->content) {
			std::cout << t.first << "-" << t.second << " ";
		}
		std::cout << std::endl;
	}

	static std::shared_ptr<NextPart>genPointer() {
		return std::make_shared<struct NextPart>(NextPart());
	}

	static std::shared_ptr<NextPart>genPointer(unsigned int partNr){
		return std::make_shared<struct NextPart>(NextPart(partNr));
	}
};

struct Folder
{
	std::string path = "";
	bool isCreated = false;
	Folder(std::string path) {
		this->path = path;
		isCreated = false;
	}
	Folder(){}
	static std::shared_ptr<struct Folder> genPointer(std::string path) {
		return std::make_shared<struct Folder>(Folder(path));
	}
};

struct File
{
	std::string name;
	std::shared_ptr<std::array<char,32>> hash = std::make_shared<std::array<char,32>>(); 
	unsigned int size;
	std::ifstream fd;
	std::unordered_map<unsigned int, std::shared_ptr<std::array<char, 4>>> crcMap;
	std::unique_ptr<NextPart> np= std::make_unique<NextPart>(NextPart());
	File(std::string name, unsigned int size){
		this->name = name;
	}
	File(){}
	File(std::string FID) {
		this->name = FID;

	}
	File(std::string FID, unsigned int size, char* hash) {
		this->name = FID;
		this->size = size;
		std::strcpy(this->hash.get()->data(), hash);
	}
	void sendCompleteFile() {
		this->np->addHoleFile(this->size%MAX_CONTENT_LENGTH == 0 ? this->size/MAX_CONTENT_LENGTH: (this->size/MAX_CONTENT_LENGTH));
	}
	static std::shared_ptr<struct File> genPointer(std::string name, unsigned int size) {
		return std::make_shared<struct File>(File(name, size));
	}
	static std::shared_ptr<struct File> genPointer(std::string name) {
		return std::make_shared<struct File>(File(name));
	}
	static std::shared_ptr<struct File> genPointer(std::string name, unsigned int size, char* hash){
		return std::make_shared<struct File>(File(name, size, hash));
	}
};

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
 };
