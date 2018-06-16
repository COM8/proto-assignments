#include "ClientToDo.h"

using namespace std;

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