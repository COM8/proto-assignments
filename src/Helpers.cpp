#include "Helpers.h"

using namespace std;

//needs fix
string hashfile(string filename) {
	ifstream fp(filename.c_str());
	stringstream ss;
	// Unable to hash file, return an empty hash.
	if (!fp.is_open()) {
		return "";
	}
	// Hashing
	uint32_t magic = 5381;
	char c;
	while (fp.get(c)) {
		magic = ((magic << 5) + magic) + c; // magic * 33 + c
	}
	ss << hex << setw(8) << setfill('0') << magic;
	return ss.str();
}


void listFiles(string folderpath)
{
	DIR *dirp;
	dirp = opendir(folderpath.c_str());
	struct dirent *directory;
	if (dirp) {
		while ((directory = readdir(dirp)) != NULL)
		{
			cout << directory->d_name << " --> " << hashfile(folderpath + "/" + directory->d_name) << endl;
		}
		closedir(dirp);
	}
}