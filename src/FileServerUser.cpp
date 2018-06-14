#include "FileServerUser.h"

using namespace std;

FileServerUser::FileServerUser(string username, string password) : clients(), USERNAME(username), PASSWORD(password)
{
    this->clientsMutex = new mutex();
    this->clientsToDo = ClientsToDo();
    this->fS = new FilesystemServer("sync/" + username + "/", &clientsToDo);
}

FileServerUser::~FileServerUser()
{
    deleteAllClients();
    fS->close();
    delete clientsMutex;
    delete fS;
}

void FileServerUser::deleteAllClients()
{
    Logger::info("Started deleting all clients for username: " + USERNAME);
    std::unique_lock<std::mutex> mlock(*clientsMutex);
    auto i = clients.begin();
    while (i != clients.end())
    {
        i->second->disconnect();
        delete i->second;
        i = clients.erase(i);
    }
    mlock.unlock();
    Logger::info("Finished deleting all clients for username: " + USERNAME);
}

ClientToDo *FileServerUser::getClientToDo(unsigned int clientId)
{
    return clientsToDo.getClientToDos(clientId);
}

void FileServerUser::removeClient(FileServerClient *client)
{
    std::unique_lock<std::mutex> mlock(*clientsMutex);
    clients.erase(client->clientId);
    delete client;
    mlock.unlock();
}

bool FileServerUser::isEmpty()
{
    std::unique_lock<std::mutex> mlock(*clientsMutex);
    bool b = clients.empty();
    mlock.unlock();

    return b;
}

void FileServerUser::clanupClients()
{
    time_t now = time(NULL);

    std::unique_lock<std::mutex> mlock(*clientsMutex);
    auto i = clients.begin();
    while (i != clients.end())
    {
        // If last message is older than 10 seconds:
        if (!i->second || i->second->getState() == fsc_error || difftime(now, i->second->lastMessageTime) > 10)
        {
            Logger::info("Removing client " + to_string(i->second->clientId) + " for inactivity.");
            delete i->second;
            i = clients.erase(i);
        }
        else
        {
            i++;
        }
    }
    mlock.unlock();
}

FileServerClient *FileServerUser::getClient(unsigned int clientId)
{
    std::unique_lock<std::mutex> mlock(*clientsMutex);
    auto c = clients.find(clientId);
    mlock.unlock();

    if (c != clients.end())
    {
        return c->second;
    }
    return NULL;
}

void FileServerUser::addClient(FileServerClient *client)
{
    std::unique_lock<std::mutex> mlock(*clientsMutex);

    if (clients[client->clientId])
    {
        delete clients[client->clientId];
    }
    clients[client->clientId] = client;
    clientsToDo.addClient(client->clientId);
    mlock.unlock();
}