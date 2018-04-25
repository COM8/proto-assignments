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

long unsigned int Filesystem::filesize(const string filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    return in.tellg(); 
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