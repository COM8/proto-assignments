#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include "FileServerClient.h"
#include "Filesystem.h"
#include "Logger.h"

class FileServerUser
{
public:
  const std::string USER_NAME;
  const std::string PASSWORD;

  FileServerUser(std::string userName, std::string password);
  FileServerUser();
  ~FileServerUser();

  FileServerClient *getClient(unsigned int clientId);
  void addClient(FileServerClient *client);

private:
  std::unordered_map<unsigned int, FileServerClient *> clients;
  std::mutex *clientsMutex;
  FilesystemServer *fS;

  void deleteAllClients();
};