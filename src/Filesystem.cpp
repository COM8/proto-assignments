#include "Filesystem.h"

namespace fs = std::experimental::filesystem;

using namespace std;

Filesystem::Filesystem(std::string p) {
	path = p;
}

bool Filesystem::exists(string path) {
	return fs::exists(path);
}

int Filesystem::genMap(){
return genMap(this->path);
}

long unsigned int Filesystem::filesize(const string FID) {
    ifstream file(FID, ifstream::ate | ifstream::binary);
    long unsigned int ret = file.tellg();  
    file.close();
    return ret;
}

int Filesystem::readFile(string FID, char* buffer, int partNr, int length) {
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

int Filesystem::genMap(string path) {
	if (Filesystem::exists(path)) {
		for (auto const &p : fs::directory_iterator(path)) {
			if (fs::is_directory(p)) {
				this->folders.push_back(p.path().string());
				genMap(p.path().string());
			}
			else {
				string temp = p.path().string();
				File *t = new File();
				t->name = temp;
				t->size = filesize(temp);
				this->files[temp] = t;
			}
		}
		return 1;
	}
	return 0;
}

string Filesystem::toString() {
	string temp = "";
	for (auto const &ent1 : this->files) {
		File *t = ent1.second;
		temp = temp + ent1.first + ": " + to_string(t->size) + " Bytes" + "\n";
	}
	return temp;
}