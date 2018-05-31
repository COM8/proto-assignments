#include "FileServerUser.h"

using namespace std;

FileServerUser::FileServerUser(string userName, string password) : clients(), USER_NAME(userName), PASSWORD(password)
{
    this->clientsMutex = new mutex();
    this->fS = new FilesystemServer("sync/" + USER_NAME + "/");
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
    Logger::info("Started deleting all clients for username: " + USER_NAME);
    auto i = clients.begin();
    while (i != clients.end())
    {
        i->second->disconnect();
        delete i->second;
        i = clients.erase(i);
    }
    Logger::info("Finished deleting all clients for username: " + USER_NAME);
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

void FileServerUser::addClient(FileServerClient * client)
{
    std::unique_lock<std::mutex> mlock(*clientsMutex);

    if (clients[client->CLIENT_ID])
    {
        delete clients[client->CLIENT_ID];
    }
    clients[client->CLIENT_ID] = client;
    mlock.unlock();
}