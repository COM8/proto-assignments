#include "Filesystem.h"

namespace fs = std::experimental::filesystem;

using namespace std;

FilesystemClient::FilesystemClient(string p)
{
	path = p;
}

bool Filesystem::exists(string path)
{
	return fs::exists(path);
}

int FilesystemClient::genMap()
{
	return genMap(this->path);
}

void Filesystem::calcSHA256(const string FID, shared_ptr<array<char,32>> buffer) {
	calcSHA256(FID, buffer.get()->data());
	}

//toDo
void Filesystem::calcSHA256(const string FID, char* buffer)
{
	Logger::debug("Caluclating md5 of "+ FID);
	MD5 m = MD5();
	int length = (int) Filesystem::filesize(FID);
	ifstream file(FID, ifstream::binary);
	if (file)
	{
		file.seekg(0, file.beg);
		char *b = new char[MAX_HASH_PART_SIZE_BYTE];
		int curPart = 0;
		while(length >= (curPart+1)*MAX_HASH_PART_SIZE_BYTE) {
			file.seekg(curPart++*MAX_HASH_PART_SIZE_BYTE, file.beg);
			file.read(b, MAX_HASH_PART_SIZE_BYTE);
			m.add(b,MAX_HASH_PART_SIZE_BYTE);
			}
		file.read(b, length%MAX_HASH_PART_SIZE_BYTE);
		m.add(b,length%MAX_HASH_PART_SIZE_BYTE);
		file.close();
		delete[] b;
		strcpy(buffer, m.getHash().c_str());
	}
	else
	{
		cerr << "error opening " << FID << "for hashing using empty hash" << endl;
		for (int i = 0; i < 32; i++)
		{
			buffer[i] = '\0';
		}
	}
}


long unsigned int Filesystem::filesize(const string FID)
{
	ifstream file(FID, ifstream::ate | ifstream::binary);
	long unsigned int ret = file.tellg();
	file.close();
	return ret;
}

//todo add crc32 here, may not update md5
int FilesystemClient::writeFilePart(std::string FID, char* buffer, unsigned int partNr, unsigned int length) {
	if(this->files[FID]) {
		this->files[FID] = File::genPointer(FID);
		}
	fstream tmp((this->path + FID), fstream::out |fstream::in | fstream::binary);
	if(tmp) {
		tmp.seekg(partNr * partLength, tmp.beg);
		tmp.write(buffer, length > partLength ? partLength : length);
		tmp.close();
		this->files[FID]->size = filesize(FID);
		calcSHA256(FID, this->files[FID]->hash);
		return partNr;
	}
	else{
		return -1;
	}
}

WorkingSet *FilesystemClient::getWorkingSet()
{
	unordered_map<string, shared_ptr<File>> files;
	list<string> deleteFolder;
	list<string> deleteFile;
	list<shared_ptr<Folder>> folders;
	genMap(this->path, &files, &folders, &deleteFile, &deleteFolder);

	// Only add the folder if it contains files:
	// ToDo: Fix me
	if (!files.empty() || !folders.empty())
	{
		shared_ptr<Folder> f = Folder::genPointer(path);
		this->folders.push_back(f);
		folders.push_back(f);
	}

	for(_List_iterator<shared_ptr<Folder> > i = this->folders.begin(); i != this->folders.end(); ++i) {
		if(!Filesystem::exists(i->get()->path)) {
			deleteFolder.push_back(i->get()->path);
			this->folders.erase(i++);
		}
	}

	for (const auto f : this->files)
	{
		if (!Filesystem::exists(f.first))
		{
			deleteFile.push_back(f.first);
			this->files.erase(f.first);
		}
	}
	return new WorkingSet(files, folders, deleteFile, deleteFolder);
}

int FilesystemClient::genMap(string path, unordered_map <string, shared_ptr<File>> *files, list<shared_ptr<Folder>> *folders, list<string> *deleteFile, list<string> *deleteFolder)
{
	if (Filesystem::exists(path))
	{
		for (auto const &p : fs::directory_iterator(path))
		{
			if (fs::is_directory(p))
			{
				if (!isInFolders(p.path().string()))
				{
					shared_ptr<Folder> f = Folder::genPointer(p.path().string());
					folders->push_back(f);
					this->folders.push_back(f);
				}
				genMap(p.path().string(), files, folders, deleteFile, deleteFolder);
			}
			else
			{
				string temp = p.path().string();
				if (!this->files[temp] == 0)
				{
					char *hash = new char[32];
					calcSHA256(temp, hash);
					if (strcmp(this->files[temp]->hash->data(), hash) == 1)
					{
						deleteFile->push_back(temp);
						this->files.erase(temp);
						shared_ptr<File> f = genFile(temp);
						this->files[temp] = f;
						(*files)[temp] = f;
					}
					delete hash;
				}
				else
				{
					shared_ptr<File> f = genFile(temp);
					f->sendCompleteFile();
					this->files[temp] = f;
					(*files)[temp] = f;
				}
			}
		}
		return 1;
	}
	return 0;
}

bool FilesystemClient::isInFolders(string path)
{
	for (const auto f : this->folders)
	{
		if (path.compare(f->path))
		{
			return true;
		}
	}
	return false;
}

int FilesystemClient::readFile(string FID, char *buffer, unsigned int partNr, bool *isLastPart)
{
	if (!(this->files[FID] == 0))
	{
		if (!this->files[FID]->isOpen)
		{
			this->files[FID]->fd = ifstream(FID, ifstream::ate | ifstream::binary);
			if (!this->files[FID]->fd)
			{
				this->files.erase(FID);
				cerr << "Error FID: " << FID << " is missing removing it" << endl;
				return -2;
			}
			else
			{
				this->files[FID]->isOpen = true;
			}
		}
		this->files[FID]->fd.seekg(partLength * partNr, this->files[FID]->fd.beg);
		int retLength = (this->files[FID]->size > (partLength * (partNr + 1))) ? partLength : this->files[FID]->size - partLength * partNr;
		retLength = retLength < 0 ? 0 : retLength;
		this->files[FID]->fd.read(buffer, retLength);
		if (partNr == ((this->files[FID]->size / partLength) + (this->files[FID]->size % partLength == 0 ? -1 : 0)))
		{
			this->files[FID]->fd.close();
			this->files[FID]->isOpen = false;
			*isLastPart = true;
		}
		else
		{
			*isLastPart = false;
		}
		return retLength;
	}
	else
	{
		return -2;
	}
}

int FilesystemClient::genMap(string path)
{
	if (Filesystem::exists(path))
	{
		for (auto const &p : fs::directory_iterator(path))
		{
			if (fs::is_directory(p))
			{
				this->folders.push_back(Folder::genPointer(p.path().string()));
				genMap(p.path().string());
			}
			else
			{
				string temp = p.path().string();
				this->files[temp] = genFile(temp);
			}
		}
		return 1;
	}
	return 0;
}


shared_ptr<File> Filesystem::genFile(string FID)
{
	shared_ptr<File> f= File::genPointer(FID);
	f->size = filesize(FID);
	calcSHA256(FID, f->hash);
	return f;
}

void FilesystemClient::close()
{
	for (auto const &ent1 : this->files)
	{
		shared_ptr<File> t = ent1.second;
		t->fd.close();
	}
}

string FilesystemClient::filesToString()
{
	string temp = "";
	for (auto const &ent1 : this->files)
	{
		shared_ptr<File> t = ent1.second;
		temp = temp + ent1.first + ": " + to_string(t->size) + " Bytes" + " Hash: " + string(t->hash.get()->data())+ "\n";
	}
	return temp;
}

string FilesystemClient::foldersToString()
{
	string temp = "";
	for (auto const &ent1 : this->folders)
	{
		temp = temp + ent1->path + "\n";
	}
	return temp;
}

FilesystemServer::FilesystemServer(string path, ClientsToDo* clientsToDo)
{
	this->path = path;
	this->clientsToDo = clientsToDo;
	if (!exists(path))
	{
		createPath();
	}
	if (!exists(path + ".csync.folders"))
	{
		genFile(".csync.folders", new char[32]);
	}
	else
	{
		readFolderFile();
	}
	if (!exists(path + ".csync.files"))
	{
		genFile(".csync.files", new char[32]);
	}
	else
	{
		readFileFile();
	}
	clearDirecotry();
}

void FilesystemServer::readFileFile()
{
	int size = filesize(this->path + ".csync.files");
	fstream tmp((this->path + ".csync.files"), fstream::in | fstream::binary);
	int currPosition = 0;
	while (currPosition < size)
	{
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

void FilesystemServer::readFolderFile()
{
	fstream tmp((this->path + ".csync.folders"), fstream::out | fstream::in | fstream::binary);
	int size = tmp.tellg();
	int currPosition = 0;
	while (currPosition < size)
	{
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

void FilesystemServer::saveFolderFile()
{
	fstream tmp((this->path + ".csync.folders"), fstream::out | fstream::in | fstream::binary | fstream::trunc);
	for (auto const &ent1 : this->folders)
	{
		tmp.write(intToArray(ent1.first.length()), 4);
		tmp.write(ent1.first.c_str(), ent1.first.length());
	}
	tmp.close();
}

void FilesystemServer::saveFileFile()
{
	fstream tmp((this->path + ".csync.files"), fstream::out | fstream::in | fstream::binary | fstream::trunc);
	for (auto const &ent1 : this->files)
	{
		tmp.write(intToArray(ent1.first.length()), 4);
		tmp.write(ent1.first.c_str(), ent1.first.length());
		unsigned int last_part = ent1.second.get()->last_part;
		tmp.write(intToArray(last_part), 4);
		tmp.write(ent1.second.get()->hash.get()->data(), 32);
	}
	tmp.close();
}

char *FilesystemServer::intToArray(unsigned int i)
{
	char *ret = new char[4];
	for (int e = 0; e < 4; e++)
	{
		ret[3 - e] = (i >> (e * 8));
	}
	return ret;
}

unsigned int FilesystemServer::charToInt(char *buffer)
{
	return static_cast<int>(buffer[0]) << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

void FilesystemServer::createPath()
{
	system(("mkdir -p " + this->path).c_str());
}

void FilesystemServer::genFolder(string path)
{
	string temp = this->path + path;
	if (this->folders[temp] == 0)
		this->folders[temp] = true;
	if (!exists(temp))
		system(("mkdir -p " + temp).c_str());
}

void FilesystemServer::delFolder(string path)
{
	string temp = this->path + path;
	this->folders.erase(temp);
	folderClean(temp);
}

void FilesystemServer::folderClean(string folder)
{
	system(("rm " + this->path + path + " -r -f").c_str());
}

void FilesystemServer::delFile(string FID)
{
	string temp = this->path + FID;
	this->files.erase(temp);
	fileClean(temp);
}

void FilesystemServer::fileClean(string file)
{
	system(("rm " + file + " -f").c_str());
}

void FilesystemServer::genFile(string FID, char *hash)
{
	if (this->files[(this->path + FID)] == 0)
	{
		this->files[(this->path + FID)] = ServerFile::genPointer(hash, 0);
	}
	fstream tmp((this->path + FID), fstream::out);
	tmp.close();
}

void FilesystemServer::clearDirecotry()
{
	if (Filesystem::exists(path))
	{
		for (auto const &p : fs::directory_iterator(path))
		{
			if (fs::is_directory(p))
			{
				if (this->folders[p.path().string()] == 0)
				{
					folderClean(p.path().string());
				}
			}
			else
			{
				if (this->files[p.path().string()] == 0)
				{
					fileClean(p.path().string());
				}
			}
		}
	}
	else
	{
		createPath();
	}
}

int FilesystemServer::writeFilePart(string FID, char *buffer, unsigned int partNr, unsigned int length, unsigned int userID)
{
	if (!exists(this->path + FID))
	{
		cerr << "File: " << FID << " is unknown by the System, but it will be created with some hash" << endl;
		genFile(FID, new char[32]);
	}
	fstream tmp((this->path + FID), fstream::out | fstream::in | fstream::binary);
	if (tmp)
	{
		tmp.seekp(partNr * partLength, tmp.beg);
		tmp.write(buffer, length > partLength ? partLength : length);
		tmp.close();
		unsigned int last_part = this->files[this->path + FID].get()->last_part;
		if (last_part + 1 == partNr)
		{
			this->files[this->path + FID].get()->last_part = last_part + 1;
		}
		return 0;
	}
	else
	{
		return -1;
	}
}

int FilesystemServer::readFile(string FID, char* buffer, unsigned int partNr) {
	if(this->files[FID]) {
		if(exists(this->path + FID)) {
			Logger::warn("adding unkonwn File to the Filesystem");
			this->files[FID] = ServerFile::genPointer();
		}else {
			Logger::error("Filesystem can't find: " + FID);
			return -2;
		}
	}else {
		if(exists(this->path + FID)) {

		}else {
			Logger::error(FID+ " is unknown by the Server");
			return -2;
		}
	}
	ifstream tmp = ifstream(this->path + FID, ifstream::ate | ifstream::binary);
	if(!tmp) {
		Logger::error("Error opening " + FID);
		return -1;
	}else {
		tmp.seekg(partLength*partNr, tmp.beg);
		int retLength = (filesize(this->path + FID) > (partLength * (partNr +1) ? partLength: filesize(this->path) - partLength * partNr));
		retLength < 0 ? 0 : retLength;
		tmp.read(buffer, retLength);
		tmp.close();
		return retLength;
	}
}

unsigned int FilesystemServer::getLastPart(string FID)
{
	if (this->files[this->path + FID] == 0)
	{
		return 0;
	}
	else
	{
		return this->files[this->path + FID].get()->last_part;
	}
}

void FilesystemServer::close()
{
	saveFolderFile();
	saveFileFile();
}
