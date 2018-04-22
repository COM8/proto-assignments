#include<filesystem.h>

bool filesystem::exists(string path) {
	return fs::exists(path);
}

int filesystem::genMap() {
	if (filesystem::exists(path)) {
		for (auto const &p : fs::directory_iterator(path)) {
			if (fs::is_directory(p)) {
				this->folders.push_back(p.path().string());
			}
			else {
				string temp = p.path().string();
				file *t = new file();
				t->name = temp;
				this->files[temp] = t;
			}
		}
		return 1;
	}
	return 0;
}

string filesystem::toString() {
	string temp = "";
	for (auto const &ent1 : this->files) {
		temp = temp + ent1.first + "\n";
	}
	return temp;
}