#pragma once

#include <mutex>
#include <unordered_map>
#include <string>
#include "ClientToDo.h"

typedef std::unordered_map<unsigned int, ClientToDo> ClientsToDoMap;

class ClientsToDo
{
public:
  ClientsToDo();
  ~ClientsToDo();
  void addClient(unsigned int clientId);
  ClientToDo *getClientToDos(unsigned int clientId);
  void addToDoForAllExcept(TodoEntry toDo, unsigned int ignoredClientId);

private:
  ClientsToDoMap clientsToDoMap;
  //std::mutex *clientsToDoMapMutex;
};