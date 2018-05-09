#include "Filesystem.h"

namespace fs = std::experimental::filesystem;

using namespace std;

FilesystemClient::FilesystemClient(std::string p) {
	path = p;
}

bool Filesystem::exists(string path) {
	return fs::exists(path);
}

int FilesystemClient::genMap(){
return genMap(this->path);
}

//toDo
void Filesystem::calcSHA256(const string FID, char* buffer){
	for(int i = 0; i < 32; i++) {
		buffer[i] = '\0';
	}
}

long unsigned int Filesystem::filesize(const string FID) {
    ifstream file(FID, ifstream::ate | ifstream::binary);
    long unsigned int ret = file.tellg();  
    file.close();
    return ret;
}

//WorkingSet should later contet the delta to last Sync trial, but in the moment it won't do it
WorkingSet* FilesystemClient::getWorkingSet() {
	WorkingSet *ret = new WorkingSet();
	ret->folders= &(this->folders);
	ret->files = &(this->files);
	return ret;
}

int FilesystemClient::readFile(string FID, char* buffer, int partNr, int length) {
	if(!(this->files[FID] == 0)) {
		if (!this->files[FID]->isOpen){
			this->files[FID]->fd = ifstream (FID, ifstream::ate | ifstream::binary);
			if(!this->files[FID]->fd){
				this->files.erase(FID);
				cerr << "Error FID: " << FID << " is missing removing it" << endl;
				return -2;
			}else {
				this->files[FID]->isOpen = true;
			}
		}
		this->files[FID]->fd.seekg(length*partNr, this->files[FID]->fd.beg);
		this->files[FID]->fd.read(buffer,length);
		return 0;
	}else {
		return -2;
	}
}

int FilesystemClient::genMap(string path) {
	if (Filesystem::exists(path)) {
		for (auto const &p : fs::directory_iterator(path)) {
			if (fs::is_directory(p)) {
				this->folders.push_back(genFolder(p.path().string()));
				genMap(p.path().string());
			}
			else {
				string temp = p.path().string();
				this->files[temp] = genFile(temp);
			}
		}
		return 1;
	}
	return 0;
}

Folder* Filesystem::genFolder(string path) {
	Folder *f = new Folder();
	f->path = path;
	return f;
}

File* Filesystem::genFile(string FID){
	File *f = new File();
	f->name = FID;
	f->size = filesize(FID);
	return f;
}

void FilesystemClient::close() {
	for (auto const &ent1 : this->files) {
		File *t = ent1.second;
		t->fd.close();
	}
}

string FilesystemClient::toString() {
	string temp = "";
	for (auto const &ent1 : this->files) {
		File *t = ent1.second;
		temp = temp + ent1.first + ": " + to_string(t->size) + " Bytes" + "\n";
	}
	return temp;
}

//SERVER

FilesystemServer::FilesystemServer(string path) {
	this->path = path;
	if(!exists(path)) {
		createPath();
	}
	if(!exists(path+".csync.folders")) {
		genFile(path+".csync.folders", new char[32]);
	}
	if(!exists(path+".csync.files")) {
		genFile(path+".csync.folders", new char[32]);
	}
}

//only quick and dirty should be changed in the future

void FilesystemServer::createPath() {
	system(("mkdir " + this->path).c_str());
}

void FilesystemServer::genFolder(string path) {
	string temp = this->path + path;
	this->folders[temp] = true;
	system(("mkdir "+temp).c_str());
}

void FilesystemServer::delFolder(string path) {
	string temp = this->path + path;
	this->folders.erase(temp);
	folderClean(temp);
}

void FilesystemServer::folderClean(string folder) {
	system(("rm "+this->path + path +  " -r -f").c_str());
}

void FilesystemServer::delFile(string FID) {
	string temp = this->path + FID;
	this->files.erase(temp);
	fileClean(temp);
}

void FilesystemServer::fileClean(string file) {
	system(("rm "+ file +  " -f").c_str());
}

void FilesystemServer::genFile(std::string FID, char* hash) {
	fstream tmp ((this->path + FID),  fstream::out);
	tmp.close();
	this->files[this->path + FID] = genServerFile(hash);

}

void FilesystemServer::clearDirecotry() {
	if (Filesystem::exists(path)) {
		for (auto const &p : fs::directory_iterator(path)) {
			if (fs::is_directory(p)) {
				if(this->folders[p.path().string()] == 0) {
					folderClean(p.path().string());
				}
			}
			else {
				if(this->files[p.path().string()] == 0) {
					fileClean(p.path().string());
				}
			}
		}
	}else {
		createPath();
	}
}


int FilesystemServer::writeFilePart(string FID, char* buffer, int partNr, int length) {
	if(!exists(this->path+FID)){
		cerr << "File: " << FID << " is unknown by the System, but it will be created with some hash" << endl;
		genFile(FID, new char[32]);
	}
	fstream tmp ((this->path + FID),  fstream::out | fstream::in | fstream::binary);
	if(tmp) {
		tmp.seekp(partNr*length, tmp.beg);
		tmp.write(buffer,length);
		tmp.close();
		if (this->files[this->path + FID]->last_part+1 == partNr) {
			this->files[this->path + FID]->last_part++;
		}
		return 0;
	}
	else {
		return -1;
	}
}

ServerFile *FilesystemServer::genServerFile(char* hash) {
	ServerFile *ret = new ServerFile();
	ret->hash = hash;
	return ret;
}
