#include "Filesystem.h"

namespace fs = std::experimental::filesystem;

using namespace std;

FilesystemClient::FilesystemClient(string p) {
	this->path = (p.c_str()[p.size()-1] == '/' ? p : (p + '/'));
	openFilesystem();
}
bool Filesystem::exists(string path) {
	return fs::exists(path);
}

int FilesystemClient::genMap() {
	return genMap(this->path);
}

void Filesystem::calcSHA256(const string FID, shared_ptr<array<char,32>> buffer) {
	calcSHA256(FID, buffer.get()->data());
	}

void Filesystem::calcSHA256(const string FID, char* buffer) {
	Logger::debug("Caluclating md5 of "+ FID);
	MD5 m = MD5();
	int length = (int) Filesystem::filesize(FID);
	ifstream file(FID, ifstream::binary);
	if (file) {
		file.seekg(0, file.beg);
		char *b = new char[MAX_HASH_PART_SIZE_BYTE];
		int curPart = 0;
		while (length >= (curPart+1)*MAX_HASH_PART_SIZE_BYTE) {
			file.seekg(curPart++*MAX_HASH_PART_SIZE_BYTE, file.beg);
			file.read(b, MAX_HASH_PART_SIZE_BYTE);
			m.add(b,MAX_HASH_PART_SIZE_BYTE);
			}
		file.read(b, length%MAX_HASH_PART_SIZE_BYTE);
		m.add(b,length%MAX_HASH_PART_SIZE_BYTE);
		file.close();
		delete[] b;
		strcpy(buffer, m.getHash().c_str());
	}else {
		cerr << "error opening " << FID << "for hashing using empty hash" << endl;
		for (int i = 0; i < 32; i++) {
			buffer[i] = '\0';
		}
	}
}

char *Filesystem::intToArray(unsigned int i) {
	char *ret = new char[4];
	for (int e = 0; e < 4; e++) {
		ret[3 - e] = (i >> (e * 8));
	}
	return ret;
}

unsigned int Filesystem::charToInt(char *buffer) {
	return static_cast<int>(buffer[0]) << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

void Filesystem::calcCRC32(char* buffer, size_t bufferLength, char crc32Bytes[CRC32::HashBytes]) {
	CRC32 crc = CRC32();
	crc.add(buffer, bufferLength);
	crc.getHash((unsigned char *)crc32Bytes);
	return;
}

long unsigned int Filesystem::filesize(const string FID) {
	ifstream file(FID, ifstream::ate | ifstream::binary);
	long unsigned int ret = file.tellg();
	file.close();
	return ret;
}

long unsigned int FilesystemClient::filesize(const string FID) {
	if(this->files[FID]) {
		if(!this->files[FID]->fd.is_open()) {
			this->files[FID]->fd.open(FID, ifstream::ate | ifstream::binary);
		} 
			long unsigned int ret = this->files[FID]->fd.tellg();
			this->files[FID]->fd.close();
			return ret;
	} else {
		ifstream file(FID, ifstream::ate | ifstream::binary);
		long unsigned int ret = file.tellg();
		file.close();
		return ret;
	}
}


void FilesystemClient::compareFiles(const string FID, shared_ptr<File> f) {
	if (exists(FID)) {
		unsigned int i = 0;
		char *buffer = new char[partLength];
		char *tmpcrc = new char[4];
		bool *n = new bool;
		int bufferLength = -1;
		while ((bufferLength = readFile(FID, buffer, i,n)) != 0){
			calcCRC32(buffer, bufferLength, tmpcrc);
			if ((f->crcMap.find(i) != f->crcMap.end()))	 {
				cout << f->crcMap[i] << endl;
				if (strcmp(tmpcrc, f->crcMap[i].get()->data())!= 0) {
					Logger::debug("Found File Delta: " + FID + ": " + to_string(i));
					f->np->addPart(i);
					shared_ptr<array<char,4>> t = make_shared<array<char,4>>();
					strcpy(t.get()->data(), tmpcrc);
					f->crcMap[i] = t;
				}
			}else {
				f->np->addPart(i);
				// Logger::debug("Added file part: "+ FID + ": " + to_string(i));
				shared_ptr<array<char,4>> t = make_shared<array<char,4>>();
				strcpy(t.get()->data(), tmpcrc);
				f->crcMap[i] = t;
			}
			i++;
		}
		delete[] buffer;
		delete n;
	}
}

int FilesystemClient::writeFilePart(const string FID, char* buffer, unsigned int partNr, unsigned int length) {
	if (this->files[FID]) {
		this->files[FID] = File::genPointer(FID);
	}
	fstream tmp((FID), fstream::out |fstream::in | fstream::binary);
	if (tmp) {
		tmp.seekg(partNr * partLength, tmp.beg);
		tmp.write(buffer, length > partLength ? partLength : length);
		tmp.close();
		this->files[FID]->size = filesize(FID);
		calcSHA256(FID, this->files[FID]->hash);
		return partNr;
	}else {
		return -1;
	}
}

WorkingSet *FilesystemClient::getWorkingSet() {
	unordered_map<string, shared_ptr<File>> files;
	list<string> deleteFolder;
	list<string> deleteFile;
	list<shared_ptr<Folder>> folders;
	genMap(this->path, &files, &folders, &deleteFile, &deleteFolder);
	if (!files.empty() || !folders.empty()) {
		shared_ptr<Folder> f = Folder::genPointer(path);
		this->folders.push_back(f);
		folders.push_back(f);
	}
	for (_List_iterator<shared_ptr<Folder> > i = this->folders.begin(); i != this->folders.end(); ++i) {
		if (!Filesystem::exists(i->get()->path)) {
			deleteFolder.push_back(i->get()->path);
			this->folders.erase(i++);
		}
	}
	for (const auto f : this->files) {
		if (!Filesystem::exists(f.first)) {
			deleteFile.push_back(f.first);
		}
	}
	for(const auto f : deleteFile) {
		this->files.erase(f);
	}
	return new WorkingSet(files, folders, deleteFile, deleteFolder);
}

int FilesystemClient::genMap(const string path, unordered_map <string, shared_ptr<File>> *files, list<shared_ptr<Folder>> *folders, list<string> *deleteFile, list<string> *deleteFolder) {
	if (Filesystem::exists(path)) {
		for (auto &p : fs::recursive_directory_iterator(path)) {
			if (fs::is_directory(p)) {
				shared_ptr<Folder> f = Folder::genPointer(p.path().string());
				folders->push_back(f);
				this->folders.push_back(f);
			} else {
				string temp = p.path().string();
				if (temp.compare(".csync.files")) {
					if (!this->files[temp] == 0) {
						char *hash = new char[32];
						calcSHA256(temp, hash);
						if (strcmp(this->files[temp]->hash->data(), hash) != 0) {
							if (this->files[temp]->size > filesize(temp)) {
								this->files[temp]->sendCompleteFile();
								deleteFile->push_back(temp);
							}else {
								compareFiles(temp, this->files[temp]);
							}
						}
						delete hash;
					}else {
						shared_ptr<File> f = genFileOBJ(temp);
						f->sendCompleteFile();
						this->files[temp] = f;
						(*files)[temp] = f;
					}
				}
			}
		}
		return 1;
	}
	return 0;
}

bool FilesystemClient::isInFolders(string path) {
	for (const auto f : this->folders) {
		if (path.compare(f->path)) {
			return true;
		}
	}
	return false;
}

//totest
void FilesystemClient::genFolder(const string path) {
	bool i = false;
	for(auto fo: this->folders) {
		if(fo->path.compare(path)) {
			i = true;
			break;
		}
	}
	if(!i) {
		shared_ptr<Folder> t = Folder::genPointer(path);
		t->isCreated = true;
		this->folders.push_back(t);
	}
	if(!fs::create_directories(this->path+path)) {
		if(!exists(this->path + path)) {
			Logger::error("Can't create " + path);
		} else {
			Logger::warn(path + " already created");
		}
	}
}

//totest
void FilesystemClient::genFile(const string FID, char *hash) {
	fstream tmp((this->path + FID), fstream::out);
	if(!tmp){
	Logger::error("Path: " + FID + " \n\tI told you, homeboy:\n\tCan't touch this.\n\tYeah, that's how we livin', and ya know:\n\tCan't touch this.\n\tLook in my eyes, man:\n\tCan't touch this.\n\tYo! Let me bust the funky lyrics.\n\tCan't touch this.");
	}
	tmp.close();
	if(this->files[FID]) {
		this->files[FID] = genFileOBJ(FID);
	}
}

//totest
void FilesystemClient::delFolder(const string path) {
	uintmax_t n= fs::remove_all(path);
	Logger::info("Deleting " + path + " ==> " + to_string(n) + " items are delted");
	for (_List_iterator<shared_ptr<Folder> > i = this->folders.begin(); i != this->folders.end(); ++i) {
		if (i->get()->path.compare(path)) {
			this->folders.erase(i);
			break;
		}
	}
}

//totest
void FilesystemClient::delFile(const string FID) {
	if(fs::remove(path)) {
		Logger::info("successfully deleted: "+ FID);
	} else {
		Logger::warn("can't delete: " + FID + "this could happen if the file was already deleted");
	}
	if(!this->files[FID]) {
		this->files.erase(FID);
	}
}

int FilesystemClient::readFile(const string FID, char *buffer, unsigned int partNr, bool *isLastPart) {
	if (!(this->files[FID] == 0)) {
		if (!this->files[FID]->fd.is_open()) {
			this->files[FID]->fd.open(FID, ifstream::ate | ifstream::binary);
			if (!this->files[FID]->fd) {
				this->files.erase(FID);
				cerr << "Error FID: " << FID << " is missing removing it" << endl;
				return -2;
			}else {
				this->files[FID]->isOpen = true;
			}
		}
		this->files[FID]->size = this->files[FID]->fd.tellg();
		this->files[FID]->fd.seekg(partLength * partNr, this->files[FID]->fd.beg);
		int retLength = (this->files[FID]->size > (partLength * (partNr + 1))) ? partLength : this->files[FID]->size - partLength * partNr;
		this->files[FID]->fd.read(buffer, retLength);
		if (partNr == ((this->files[FID]->size / partLength) + (this->files[FID]->size % partLength == 0 ? -1 : 0)) || retLength == 0) {
			this->files[FID]->fd.close();
			this->files[FID]->isOpen = false;
			*isLastPart = true;
		}else {
			*isLastPart = false;
		}
		return retLength;
	}else {
		return -2;
	}
}

int FilesystemClient::genMap(string path) {
	if (Filesystem::exists(path)) {
		for (auto const &p : fs::recursive_directory_iterator(path)) {
			if (fs::is_directory(p)) {
				this->folders.push_back(Folder::genPointer(p.path().string()));
			}else {
				string temp = p.path().string();
				this->files[temp] = genFileOBJ(temp);
			}
		}
		return 1;
	}
	return 0;
}


shared_ptr<File> FilesystemClient::genFileOBJ(const string FID) {
	shared_ptr<File> f= File::genPointer(FID);
	f->size = filesize(FID);
	calcSHA256(FID, f->hash);
	return f;
}

void FilesystemClient::saveFilesystem() {
	fstream tmp(this->path + ".csync.files", fstream::out | fstream::in | fstream::binary | fstream::trunc);
	if (tmp) {
		for (auto f: this->files) {
			char *nameLen = intToArray(f.first.length());
			tmp.write(nameLen, 4);
			tmp.write(f.first.c_str(), f.first.length());
			char *size = intToArray(f.second->size);
			tmp.write(size, 4);
			tmp.write(f.second->hash.get()->data(), 32);
			char *crcsize = intToArray(f.second->crcMap.size());
			for (auto d : f.second->crcMap) {
				char* crcNumber = intToArray(d.first);
				tmp.write(crcNumber, 4);
				tmp.write(d.second.get()->data(), 4);
				delete[] crcNumber;
			}
			delete[] nameLen;
			delete[] size;
			delete[] crcsize;
		}
		tmp.close();
	}else {
		Logger::error("can't open Filesystem file");
	}
}

void FilesystemClient::openFilesystem() {
	int size = filesize(this->path + ".csync.files");
	fstream tmp(this->path + ".csync.files", fstream::in | fstream::binary);
	if (tmp) {
		int currPosition = 0;
		char *intVar = new char[4];
		while(currPosition < size) {
			tmp.read(intVar, 4);
			int nameLength = charToInt(intVar);
			char *FID = new char[nameLength];
			tmp.read(FID, nameLength);
			tmp.read(intVar,4);
			int size = charToInt(intVar);
			char *Hash = new char[32];
			tmp.read(Hash, 32);
			shared_ptr<File> t = File::genPointer(string(FID), size, Hash);
			tmp.read(intVar, 4);
			int crcsize = charToInt(intVar);
			char* crcValue = new char[4];
			currPosition += 44;
			for (int i = 0; i < crcsize; i++) {
				tmp.read(intVar, 4);
				int crcN = charToInt(intVar);
				tmp.read(crcValue, 4);
				t->crcMap[crcN] = make_shared<array<char, 4>>();
				strcpy(t->crcMap[crcN].get()->data(), crcValue);
				currPosition += 8;
				if (currPosition >= size) {
					break;
				}
			}
			this->files[string(FID)] = t;
			delete[] crcValue;
			delete[] Hash;
			delete[] FID;
		}
		delete[] intVar;
		tmp.close();
	}else {
		Logger::warn("there is no .csync.files, can't restore previouse state");
	}
}

void FilesystemClient::close() {
	for (auto const &ent1 : this->files) {
		shared_ptr<File> t = ent1.second;
		t->fd.close();
	}
	saveFilesystem();
}

string FilesystemClient::filesToString() {
	string temp = "";
	for (auto const &ent1 : this->files) {	
		shared_ptr<File> t = ent1.second;
		temp = temp + ent1.first + ": " + to_string(t->size) + " Bytes" + " Hash: " + string(t->hash.get()->data())+ "\n";
	}
	return temp;
}

string FilesystemClient::foldersToString() {
	string temp = "";
	for (auto const &ent1 : this->folders) {
		temp = temp + ent1->path + "\n";
	}
	return temp;
}

FilesystemServer::FilesystemServer() {
}

void FilesystemServer::init(string path, ClientsToDo* clientsToDo) {
	this->path = path.c_str()[path.size()-1] == '/' ? path: path + '/';
	this->clientsToDo = clientsToDo;
	if (!exists(path)) {
		if(createPath()) {
			Logger::info("created sync path");
		}else {
			Logger::error("can't create sync path terminating");
			exit(-1);
		}
	}
	if (!exists(path + ".csync.folders")) {
		genFile(".csync.folders", new char[32]);
	}else {
		readFolderFile();
	}
	if (!exists(path + ".csync.files")) {
		genFile(".csync.files", new char[32]);
	}else {
		readFileFile();
	}
	clearDirecotry();
}

void FilesystemServer::readFileFile() {
	int size = filesize(this->path + ".csync.files");
	fstream tmp((this->path + ".csync.files"), fstream::in | fstream::binary);
	int currPosition = 0;
	while (currPosition < size) {
		char *length = new char[4];
		tmp.read(length, 4);
		int l = charToInt(length);
		char *name = new char[l];
		tmp.read(name, l);
		tmp.read(length, 4);
		int last_part = charToInt(length);
		this->files[string(name)] = ServerFile::genPointer(last_part);
		tmp.read(this->files[string(name)]->hash.get()->data(), 32);
		currPosition += l + 40;
		delete[] length;
		delete[] name;
	}
	tmp.close();
}

void FilesystemServer::readFolderFile() {
	fstream tmp((this->path + ".csync.folders"), fstream::out | fstream::in | fstream::binary);
	int size = tmp.tellg();
	int currPosition = 0;
	while (currPosition < size) {
		char *length = new char[4];
		tmp.read(length, 4);
		int l = charToInt(length);
		char *name = new char[l];
		tmp.read(name, l);
		this->folders[string(name)] = true;
		currPosition += l + 4;
		delete [] length;
		delete[] name;
	}
	tmp.close();
}

void FilesystemServer::saveFolderFile() {
	fstream tmp((this->path + ".csync.folders"), fstream::out | fstream::in | fstream::binary | fstream::trunc);
	for (auto const &ent1 : this->folders) {
		tmp.write(intToArray(ent1.first.length()), 4);
		tmp.write(ent1.first.c_str(), ent1.first.length());
	}
	tmp.close();
}

//totest fixed memleak
void FilesystemServer::saveFileFile() {
	fstream tmp((this->path + ".csync.files"), fstream::out | fstream::in | fstream::binary | fstream::trunc);
	for (auto const &ent1 : this->files) {
		if (!(ent1.first.compare(this->path+".csync.files") || ent1.first.compare(this->path+".csync.folders"))) {
			char *fidLen = intToArray(ent1.first.length());
			tmp.write(fidLen, 4);
			delete[] fidLen;
			tmp.write(ent1.first.c_str(), ent1.first.length());
			char *lPart = intToArray(ent1.second->last_part);
			tmp.write(lPart, 4);
			delete[] lPart;
			tmp.write(ent1.second.get()->hash->data(), 32);
		}else {
		}
	}
	tmp.close();
}


bool FilesystemServer::createPath() {
	return fs::create_directories(this->path);
}

//totest switched to filesystemlibrary for folder creation
void FilesystemServer::genFolder(string path, unsigned int clientID) {
	if (this->folders[path] == 0){
			this->folders[path] = true;
		}
		if(fs::create_directories(this->path+path)) {
			Logger::error("created " + path);
			TodoEntry t = TodoEntry();
			t.createFolder(path);
			this->clientsToDo->addToDoForAllExcept(t, clientID);
		}else {
			if(!exists(this->path + path)) {
				Logger::error("Can't create " + path);
			} else {
				Logger::warn(path + " already created");
			}
		}
}

void FilesystemServer::delFolder(string path, unsigned int clientID) {
	string temp = this->path + path;
	this->folders.erase(temp);
	folderClean(temp);
	TodoEntry t = TodoEntry();
	t.delFolder(path);
	this->clientsToDo->addToDoForAllExcept(t, clientID);
}

//totest switched from legacy system method to new fs method
//toimpl remove from file structure
void FilesystemServer::folderClean(string folder) {
	uintmax_t n= fs::remove_all(this->path + folder);
	Logger::info("Deleting " + folder + " ==> " + to_string(n) + " items are delted");
	//system(("rm " + this->path + path + " -r -f").c_str());
}

//toimpl remove from file structure
void FilesystemServer::delFile(string FID, unsigned int clientID) {
	string temp = this->path + FID;
	this->files.erase(temp);
	fileClean(temp);
	TodoEntry t = TodoEntry();
	t.delFile(FID);
	this->clientsToDo->addToDoForAllExcept(t, clientID);
}

//totest switched from legacy system method to filesystem operation
void FilesystemServer::fileClean(string file) {
	if(fs::remove(file)) {
		Logger::info("successfully deleted: "+ file);
	} else {
		Logger::warn("can't delete: " + file + " this could happen if the file was already deleted");
	}
	//system(("rm '" + file + "' -f").c_str());
}

//totest switch back to old method
void FilesystemServer::genFile(string FID, char *hash, unsigned int clientID) {
	genFile(FID, hash);
	TodoEntry t = TodoEntry();
	t.createFile(FID, (unsigned char*) this->files[FID]->hash.get()->data());
	this->clientsToDo->addToDoForAllExcept(t, clientID);
}

void FilesystemServer::genFile(string FID, char* hash) {
	if (this->files[FID] == 0) {
		this->files[FID] = ServerFile::genPointer(hash, 0);
	}
	//system (("touch '" + (this->path + FID) + "'").c_str());
	fstream tmp((this->path + FID), fstream::out);
	if(!tmp){
	Logger::error("Path: " + FID + " \n\tI told you, homeboy:\n\tCan't touch this.\n\tYeah, that's how we livin', and ya know:\n\tCan't touch this.\n\tLook in my eyes, man:\n\tCan't touch this.\n\tYo! Let me bust the funky lyrics.\n\tCan't touch this.");
	}
	tmp.close();
}

void FilesystemServer::clearDirecotry() {
	if (Filesystem::exists(path)) {
		for (auto const &p : fs::directory_iterator(path)) {
			if (fs::is_directory(p)) {
				if (this->folders[p.path().string()] == 0) {
					folderClean(p.path().string());
				}
			}else {
				if (this->files[p.path().string()] == 0) {
					if((!p.path().string().compare(".csync.folders"))&&(!p.path().string().compare(".csync.files")))
						fileClean(p.path().string());
				}
			}
		}
	}else {
		createPath();
	}
}

int FilesystemServer::writeFilePart(string FID, char *buffer, unsigned int partNr, unsigned int length, unsigned int clientID) {
	if (!exists(this->path + FID)) {
		cerr << "File: " << FID << " is unknown by the System, but it will be created with some hash" << endl;
		this->files[FID] = ServerFile::genPointer(0);
	}
	fstream tmp((this->path + FID), fstream::out | fstream::in | fstream::binary);
	if (tmp) {
		tmp.seekp(partNr * partLength, tmp.beg);
		tmp.write(buffer, length > partLength ? partLength : length);
		tmp.close();
		unsigned int last_part = this->files[FID].get()->last_part;
		if (last_part + 1 == partNr) {
			this->files[FID].get()->last_part = last_part + 1;
		}
		TodoEntry t = TodoEntry();
		t.transFile(FID, partNr, (unsigned char*) this->files[FID]->hash.get()->data());
		this->clientsToDo->addToDoForAllExcept(t, clientID);
		return 0;
	}else {
		return -1;
	}
}

int FilesystemServer::readFile(string FID, char* buffer, unsigned int partNr) {
	if (this->files[FID]) {
		if (exists(this->path + FID)) {
			Logger::warn("adding unkonwn File to the Filesystem");
			this->files[FID] = ServerFile::genPointer(0);
		}else {
			Logger::error("Filesystem can't find: " + FID);
			return -2;
		}
	}else {
		if (exists(this->path + FID)) {
		}else {
			Logger::error(FID+ " is unknown by the Server");
			return -2;
		}
	}
	ifstream tmp = ifstream(this->path + FID, ifstream::ate | ifstream::binary);
	if (!tmp) {
		Logger::error("Error opening " + FID);
		return -1;
	}else {
		tmp.seekg(partLength*partNr, tmp.beg);
		int retLength = (filesize(this->path + FID) > (partLength * (partNr +1)) ? partLength: filesize(this->path) - partLength * partNr);
		retLength= retLength < 0 ? 0 : retLength;
		tmp.read(buffer, retLength);
		tmp.close();
		return retLength;
	}
}

unsigned int FilesystemServer::getLastPart(string FID) {
	if (this->files[FID] == 0) {
		return 0;
	}else {
		return this->files[FID].get()->last_part;
	}
}

void FilesystemServer::close() {
	saveFolderFile();
	saveFileFile();
}
