#include <iostream>
#include <string>
#include <cstring>

using namespace std;

enum mode {server, client, None};

const string helpstring = "t";

struct arg {
    mode type = None;
    unsigned int port = 0;
    string host, dir = "";
};

struct arg* parseParameter(int argc, char* argv[]);

int main(int argc, char* argv[]);
