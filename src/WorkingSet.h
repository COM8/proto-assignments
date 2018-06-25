#pragma once

#include <list>
#include <string>
#include <mutex>
#include <memory>
#include <cstring>
#include <unordered_map>

#include "Logger.h"
#include "Filestructures.h"
 #include "net/FileCreationMessage.h"

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

  public:
    WorkingSet(std::unordered_map<std::string, std::shared_ptr<File>> files, std::list<std::shared_ptr<Folder>> folders, std::list<std::string> deleteFile, std::list<std::string> deleteFolder)
    {
        this->files = files;
        this->folders = folders;
        this->deleteFile = deleteFile;
        this->deleteFolder = deleteFolder;

        this->curFile = NULL;
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

    void deleteCurFile() {
        if(curFileExists()) {
            files.erase(curFile->first);
            curFile = NULL;
        }
    }

    bool curFileExists() {
        bool b = false;
        curFileMutex.lock();
        if(curFile) {
            b = true;
        }
        curFileMutex.unlock();
        return b;
    }

    std::string getCurFID() {
        curFileMutex.lock();
        std::string temp = "";
        if(this->curFile) {
            temp = this->curFile->first;
        }
        curFileMutex.unlock();
        return temp;
    }

    int getCurNextPart() {
        curFileMutex.lock();
        if(!this->curFile){
            curFileMutex.unlock();
            return -1;
        }
        int ret;
        ret = this->curFile->second->np->getNextPart();
        curFileMutex.unlock();
        return ret;
    }

    std::shared_ptr<File> getCurFileFile() {
        curFileMutex.lock();
        if (!this->curFile) {
            return NULL;
        }
        std::shared_ptr<File> t = curFile->second;
        return t;
    }

    void unlockCurFile()
    {
        curFileMutex.unlock();
    }

    /*void setCurFilePartNr(int i)
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
    }*/
};
