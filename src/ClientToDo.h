#pragma once

#include <mutex>
#include <unordered_map>
#include <string>
#include "WorkingSet.h"

struct TodoEntry
{
  net::FileType type;
  NextPart np;
  std::string fid;
  unsigned char *hash;
  void transFile(std::string fid, unsigned int n, unsigned char *hash) {
    this->fid = fid;
    this->type = net::FileType::ft_file;
    this->np = NextPart(n);
    this->hash = hash;
  }
  void createFolder(std::string fid) {
    this->fid = fid;
    this->type = net::FileType::ft_folder;
  }
  void delFolder(std::string fid) {
    this->fid = fid;
    this->type = net::FileType::ft_del_folder;
  }
  void delFile(std::string fid) {
    this->fid = fid;
    this->type = net::FileType::ft_del_file;
  }
  void createFile(std::string fid, unsigned char *hash) {
    this->fid = fid;
    this->type = net::FileType::ft_none;
    this->hash = hash;
  }
};

typedef std::unordered_map<std::string, TodoEntry> ClientToDoMap;

class ClientToDo
{
public:
  ClientToDo();
  ~ClientToDo();
  void addToDo(TodoEntry toDo);
  void removeToDo(std::string fid);
  TodoEntry *getNext();
  bool isEmpty();

private:
  ClientToDoMap clientToDoMap;
  std::mutex *clientToDoMapMutex;
};