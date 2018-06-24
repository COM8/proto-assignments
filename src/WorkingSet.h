#pragma once

#include <unordered_map>
#include <list>
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <cstring>
#include "Logger.h"
#include "Consts.h"
#include "net/FileCreationMessage.h"

struct NextPart{
    std::list<std::pair<unsigned int, unsigned int>> content;

    NextPart(){};

    NextPart(unsigned int partNr) {
        this->addPart(partNr);
    }

    bool isEmpty() {
        return this->content.empty();
    }

    unsigned int getNextPart() {
        if(!this->isEmpty()){
            return this->content.begin()->first;
        }
        return -1;
    }

    void addHoleFile(unsigned int length) {
        this->content.clear();
        this->content.push_back(std::pair<unsigned int, unsigned int> (0, length));
    }
    void addPart(unsigned int partNr) {
        if(!this->isEmpty()){
            for(std::list<std::pair<unsigned int, unsigned int>>::iterator i = content.begin(); i != content.end(); ++i){
                if(i->first <= partNr && partNr<= i->second) {
                    return;
                }
                if(partNr < i->first) {
                    if(partNr + 1 == i->first) {
                        i->first = partNr;
                        return;
                    }
                    else {
                        this->content.insert(i, std::pair<unsigned int, unsigned int>(partNr, partNr));
                        return;
                    }
                }else {
                    std::list<std::pair<unsigned int, unsigned int>>::iterator next = ++i;
                    --i;    
                    if(next->first <= partNr && partNr <= next->second) {
                        return;
                    }else {
                        if(i->second < partNr && partNr < next->first) {
                            bool c = false;
                            if(i->second+1== partNr) {
                                i->second = partNr;
                                c = true;
                            }else {
                                if(next->first -1 == partNr) {
                                    next->first = partNr;
                                    c = true;
                                }
                                else {
                                    this->content.insert(next,std::pair<unsigned int, unsigned int>(partNr, partNr));
                                    return;
                                }
                            }
                            if(c) {
                                if(i->second == next->first || i->second +1 == next->first) {
                                    this->content.insert(i, std::pair<unsigned int, unsigned int>(i->first, next->second));
                                    this->content.erase(i);
                                    this->content.erase(next);
                                    return;
                                }
                                return;
                                }
                            }
                        }
                }
            }
            std::list<std::pair<unsigned int, unsigned int>>::iterator i = content.begin();
            if(i->second + 1 == partNr) {
                i->second = partNr;
                //this->content.push_back(std::pair<unsigned int, unsigned int>(partNr, partNr));
                return;
            }else {
                this->content.push_back(std::pair<unsigned int, unsigned int>(partNr, partNr));
                return;
            }
        }
        else {
            this->content.push_back(std::pair<unsigned int, unsigned int>(partNr, partNr));
            return;
        }
    }

    int acknowledgePart(unsigned int partNr) {
        if(this->isEmpty()) {
            return -1;
        }
        std::list<std::pair<unsigned int, unsigned int>>::iterator i = this->content.begin();
        if(i->first == partNr) {
            if(i->first == i->second) {
                this->content.erase(i);
            }else {
                i->first = i->first +1;
            }
            return 0;
        }
        return -1;
    }

    void printNexPart() {
        for(std::pair<unsigned int, unsigned int> t: this->content) {
            std::cout << t.first << "-" << t.second << " ";
        }
        std::cout << std::endl;
    }

    static std::shared_ptr<NextPart>genPointer() {
        return std::make_shared<struct NextPart>(NextPart());
    }

    static std::shared_ptr<NextPart>genPointer(unsigned int partNr){
        return std::make_shared<struct NextPart>(NextPart(partNr));
    }
};

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
    unsigned int size;
    std::ifstream fd;
    std::unordered_map<unsigned int, std::shared_ptr<std::array<char, 4>>> crcMap;
    std::unique_ptr<NextPart> np= std::make_unique<NextPart>(NextPart());
    File(std::string name, unsigned int size){
        this->name = name;
    }
    File(){}
    File(std::string FID) {
        this->name = FID;

    }
    File(std::string FID, unsigned int size, char* hash) {
        this->name = FID;
        this->size = size;
        std::strcpy(this->hash.get()->data(), hash);
    }
    void sendCompleteFile() {
        this->np->addHoleFile(this->size%MAX_CONTENT_LENGTH == 0 ? this->size/MAX_CONTENT_LENGTH: (this->size/MAX_CONTENT_LENGTH));
    }
    static std::shared_ptr<struct File> genPointer(std::string name, unsigned int size) {
        return std::make_shared<struct File>(File(name, size));
    }
    static std::shared_ptr<struct File> genPointer(std::string name) {
        return std::make_shared<struct File>(File(name));
    }
    static std::shared_ptr<struct File> genPointer(std::string name, unsigned int size, char* hash){
        return std::make_shared<struct File>(File(name, size, hash));
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

    bool curFileExists() {
        bool t = true;
        if(!this->curFile) {
            t = false;
        }
        return t;
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
