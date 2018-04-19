#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

namespace fs = std::experimental::filesystem;
using namespace std;

//in the moment the struct item is useless but can later be used to save important information
struct file {
    string name;
};

class filesystem {
    private:
        string path;
        unordered_map <string, file*> files; 

    public:
        filesystem(string p){
            path = p;
        };
        static bool exists(string path);
        int genMap();
        string toString();
};