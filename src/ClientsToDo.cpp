#include "ClientsToDo.h"

using namespace std;
using namespace net;

ClientsToDo::ClientsToDo()
{
    this->clientsToDoMap = ClientsToDoMap();
    this->clientsToDoMapMutex = new mutex();
}

ClientsToDo::~ClientsToDo()
{
    delete clientsToDoMapMutex;
}

void ClientsToDo::addClient(unsigned int clientId)
{
    unique_lock<mutex> mlock(*clientsToDoMapMutex);
    ClientsToDoMap::iterator iter = clientsToDoMap.find(clientId);

    if (iter == clientsToDoMap.end())
    {
        clientsToDoMap[clientId] = ClientToDo();
    }
    mlock.unlock();
}

ClientToDo *ClientsToDo::getClientToDos(unsigned int clientId)
{
    unique_lock<mutex> mlock(*clientsToDoMapMutex);
    ClientsToDoMap::iterator iter = clientsToDoMap.find(clientId);
    if (iter == clientsToDoMap.end())
    {
        mlock.unlock();
        return NULL;
    }
    ClientToDo *toDo = &(iter->second);
    mlock.unlock();
    return toDo;
}

void ClientsToDo::addToDoForAllExcept(TodoEntry toDo, unsigned int ignoredClientId)
{
    unique_lock<mutex> mlock(*clientsToDoMapMutex);
    for (ClientsToDoMap::iterator iter = clientsToDoMap.begin(); iter != clientsToDoMap.end(); iter++)
    {
        if (iter->first != ignoredClientId)
        {
            iter->second.addToDo(toDo);
        }
    }
    mlock.unlock();
}

// ---------------------------------------ClientToDo:---------------------------------------
ClientToDo::ClientToDo()
{
    this->clientToDoMap = ClientToDoMap();
    this->clientToDoMapMutex = new mutex();
}

ClientToDo::~ClientToDo()
{
    delete clientToDoMapMutex;
}

void ClientToDo::addToDo(TodoEntry toDo)
{
    unique_lock<mutex> mlock(*clientToDoMapMutex);
    ClientToDoMap::iterator iter = clientToDoMap.find(toDo.fid);
    if (iter == clientToDoMap.end())
    {
        clientToDoMap[toDo.fid] = toDo;
    }
    else
    {
        while (!toDo.np.isempty())
        {
            iter->second.np.addPart(toDo.np.getNextPart());
        }
    }
    mlock.unlock();
}

bool ClientToDo::isEmpty()
{
    unique_lock<mutex> mlock(*clientToDoMapMutex);
    bool result = clientToDoMap.empty();
    mlock.unlock();
    return result;
}

TodoEntry *ClientToDo::getNext()
{
    unique_lock<mutex> mlock(*clientToDoMapMutex);
    ClientToDoMap::iterator iter = clientToDoMap.begin();
    if (iter == clientToDoMap.end())
    {
        mlock.unlock();
        return NULL;
    }
    TodoEntry *entry = &(iter->second);
    mlock.unlock();
    return entry;
}

void ClientToDo::removeToDo(string fid)
{
    unique_lock<mutex> mlock(*clientToDoMapMutex);
    clientToDoMap.erase(fid);
    mlock.unlock();
}