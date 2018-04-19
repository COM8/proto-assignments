#include<filesystem.h>

bool filesystem::exists(string path) {
    return fs::exists(path);
}

//temporary method for testing the filesystem module should be remove when implementing the file
int test() {
    string path = "./";
    if(filesystem::exists(path)){
        for (auto & p: fs::directory_iterator(path)) {
            cout << p << endl;
        }
        return 0;
    }
    return -1;
}