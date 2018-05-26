#pragma once

#include <unordered_map>
#include <list>
#include <string>
#include <fstream>
#include <mutex>

struct Folder
{
    std::string path = "";
    bool isCreated = false;
};

struct File
{
    std::string name;
    char *hash = new char[32];
    bool isOpen = false;
    unsigned int size = 0;
    std::ifstream fd;
    unsigned last_part = 0;
};

class WorkingSet
{
  private:
    std::mutex foldersMutex;
    std::mutex filesMutex;
    std::mutex delFolderMutex;
    std::mutex delFileMutex;
    std::mutex curFileMutex;
    std::mutex curFileParMutex;
    std::list<Folder *> folders;
    std::unordered_map<std::string, File *> files;
    std::list<std::string> deleteFolder;
    std::list<std::string> deleteFile;
    std::pair<std::string, File *> *curFile;
    int curFilePartNr = -1;

  public:
    WorkingSet(std::unordered_map<std::string, File *> files, std::list<Folder *> folders, std::list<std::string> deleteFile, std::list<std::string> deleteFolder)
    {
        this->files = files;
        this->folders = folders;
        this->deleteFile = deleteFile;
        this->deleteFolder = deleteFolder;
    }

    void addDeleteFolder(std::string path)
    {
        delFolderMutex.lock();
        this->deleteFolder.push_back(path);
        delFolderMutex.unlock();
    }

    void addDeleteFile(std::string FID)
    {
        delFileMutex.lock();
        this->deleteFile.push_back(FID);
        delFileMutex.unlock();
    }

    void addFolder(Folder *f)
    {
        foldersMutex.lock();
        this->folders.push_back(f);
        foldersMutex.unlock();
    }

    void addFile(std::string FID, File *f)
    {
        filesMutex.lock();
        this->files[FID] = f;
    }

    std::unordered_map<std::string, File *> *getFiles()
    {
        filesMutex.lock();
        return &this->files;
    }

    void unlockFiles()
    {
        filesMutex.unlock();
    }

    std::list<Folder *> *getFolders()
    {
        foldersMutex.lock();
        return &this->folders;
    }

    void unlockFolders()
    {
        foldersMutex.unlock();
    }

    std::list<std::string> *getDelFolders()
    {
        delFolderMutex.lock();
        return &deleteFolder;
    }

    bool isEmpty()
    {
        return true;
    }

    void unlockdelFolders()
    {
        delFolderMutex.unlock();
    }

    std::list<std::string> *getDelFiles()
    {
        delFileMutex.lock();
        return &deleteFile;
    }

    void unlockdelFiles()
    {
        delFileMutex.unlock();
    }

    void setCurFile(std::pair<std::string, File *> *in)
    {
        curFileMutex.lock();
        this->curFile = in;
        curFileMutex.unlock();
    }

    std::pair<std::string, File *> *getCurFile()
    {
        curFileMutex.lock();
        return curFile;
    }

    void unlockCurFile()
    {
        curFileMutex.unlock();
    }

    void setCurFilePartNr(int i)
    {
        curFileParMutex.lock();
        curFilePartNr = i;
        curFileParMutex.unlock();
    }

    int getCurFilePartNr()
    {
        curFileParMutex.lock();
        int i = curFilePartNr;
        curFileParMutex.unlock();
        return i;
    }
};