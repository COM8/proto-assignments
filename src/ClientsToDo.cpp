#include "ClientsToDo.h"

using namespace std;
using namespace net;

ClientsToDo::ClientsToDo()
{
    this->clientsToDoMap = ClientsToDoMap();
    //this->clientsToDoMapMutex = new mutex();
}

ClientsToDo::~ClientsToDo()
{
    //delete clientsToDoMapMutex;
}

void ClientsToDo::addClient(unsigned int clientId)
{
    //unique_lock<mutex> mlock(*clientsToDoMapMutex);
    ClientsToDoMap::iterator iter = clientsToDoMap.find(clientId);

    if (iter == clientsToDoMap.end())
    {
        clientsToDoMap[clientId] = ClientToDo();
    }
    //mlock.unlock();
}

ClientToDo *ClientsToDo::getClientToDos(unsigned int clientId)
{
    //unique_lock<mutex> mlock(*clientsToDoMapMutex);
    ClientsToDoMap::iterator iter = clientsToDoMap.find(clientId);
    if (iter == clientsToDoMap.end())
    {
        //mlock.unlock();
        return NULL;
    }
    ClientToDo *toDo = &(iter->second);
    //mlock.unlock();
    return toDo;
}

void ClientsToDo::addToDoForAllExcept(TodoEntry toDo, unsigned int ignoredClientId)
{
    //unique_lock<mutex> mlock(*clientsToDoMapMutex);
    for (ClientsToDoMap::iterator iter = clientsToDoMap.begin(); iter != clientsToDoMap.end(); iter++)
    {
        if (iter->first != ignoredClientId)
        {
            iter->second.addToDo(toDo);
        }
    }
    //mlock.unlock();
}