#include <experimental/filesystem>
#include <iostream>
#include <string>

namespace fs = std::experimental::filesystem;
using namespace std;

class filesystem {
    
    public:
        static bool exists(string path);
};