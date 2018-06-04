#include <unordered_map>
#include <list>
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <Logger.h>

#pragma once

struct Folder
{
    std::string path = "";
    bool isCreated = false;
    Folder(std::string path) {
        this->path = path;
        isCreated = false;
    }
    Folder(){}
    static std::shared_ptr<struct Folder> genPointer(std::string path) {
        return std::make_shared<struct Folder>(Folder(path));
    }
};

struct File
{
    std::string name;
    std::shared_ptr<std::array<char,32>> hash = std::make_shared<std::array<char,32>>(); 
    bool isOpen = false;
    unsigned int size;
    std::ifstream fd;
    unsigned last_part;
    File(std::string name, unsigned int size){
        this->name = name;
        this->hash = hash;
        this->size = size;
    }
    File(){}
    File(std::string FID) {
        this->name = FID;

    }
    static std::shared_ptr<struct File> genPointer(std::string name, unsigned int size) {
        return std::make_shared<struct File>(File(name, size));
    }
    static std::shared_ptr<struct File> genPointer(std::string name) {
        return std::make_shared<struct File>(File(name));
    }
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
    std::list<std::shared_ptr<Folder>> folders;
    std::unordered_map<std::string, std::shared_ptr<File>> files;
    std::list<std::string> deleteFolder;
    std::list<std::string> deleteFile;
    std::unique_ptr<std::pair<std::string, std::shared_ptr<File>>> curFile;
    int curFilePartNr = -1;

  public:
    WorkingSet(std::unordered_map<std::string, std::shared_ptr<File>> files, std::list<std::shared_ptr<Folder>> folders, std::list<std::string> deleteFile, std::list<std::string> deleteFolder)
    {
        this->files = files;
        this->folders = folders;
        this->deleteFile = deleteFile;
        this->deleteFolder = deleteFolder;
    }

    bool isEmpty()
    {
        filesMutex.lock();
        if (!this->files.empty())
        {
            filesMutex.unlock();
            return false;
        }
        filesMutex.unlock();

        foldersMutex.lock();
        if (!this->folders.empty())
        {
            foldersMutex.unlock();
            return false;
        }
        foldersMutex.unlock();

        delFileMutex.lock();
        if (!this->deleteFile.empty())
        {
            delFileMutex.unlock();
            return false;
        }
        delFileMutex.unlock();

        delFolderMutex.lock();
        if (!this->deleteFolder.empty())
        {
            delFolderMutex.unlock();
            return false;
        }
        delFolderMutex.unlock();
        return true;
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

    void addFolder(std::shared_ptr<Folder> f)
    {
        foldersMutex.lock();
        this->folders.push_back(f);
        foldersMutex.unlock();
    }

    void addFile(std::string FID, std::shared_ptr<File> f)
    {
        filesMutex.lock();
        this->files[FID] = f;
    }

    std::unordered_map<std::string, std::shared_ptr<File>> *getFiles()
    {
        filesMutex.lock();
        return &this->files;
    }

    void unlockFiles()
    {
        filesMutex.unlock();
    }

    std::list<std::shared_ptr<Folder>> *getFolders()
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

    void unlockDelFolders()
    {
        delFolderMutex.unlock();
    }

    std::list<std::string> *getDelFiles()
    {
        delFileMutex.lock();
        return &deleteFile;
    }

    void unlockDelFiles()
    {
        delFileMutex.unlock();
    }

    void setCurFile(std::string FID, std::shared_ptr<File>f)
    {
        curFileMutex.lock();
        this->curFile = std::make_unique<std::pair<std::string, std::shared_ptr<File>>>(std::pair<std::string, std::shared_ptr<File>>(FID,f));
        curFileMutex.unlock();
    }

    std::string getCurFID() {
        curFileMutex.lock();
        std::string temp = this->curFile->first;
        curFileMutex.unlock();
        return temp;
    }

    std::shared_ptr<File> getCurFileFile() {
        curFileMutex.lock();
        std::shared_ptr<File> t = curFile->second;
        return t;
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