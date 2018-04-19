#include <iostream>
#include <string>
#include <cstring>
#include <helpers.cpp>
#include <filesystem.cpp>

using namespace std;

const int DEFAULT_PORT=1234;
enum mode {server, client, None};

const string helpString = "Client:\tcsync [-h <hostname|ip-addr>] [-p <port>] [-f <directory-path>]\nServer:\tcsyc [-s] [-p <port>]\n";

struct arg {
    mode type = None;
    unsigned int port = DEFAULT_PORT;
    string host = "", dir = "";
};

struct arg* parseParameter(int argc, char* argv[]);
void launchServer(unsigned int port);
void launchClient(unsigned int port, string host, string dir);
int main(int argc, char* argv[]);
