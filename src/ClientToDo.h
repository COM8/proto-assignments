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