#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include "Filesystem.h"
#include "FileClient2.h"
#include "FileServer2.h"

const unsigned int DEFAULT_PORT = 1234;
const unsigned int DEFAULT_MAX_PPS = 0;
enum mode { server, client, none };

const std::string helpString = "Client:\tcsync [-h <hostname|ip-addr>] [-p <port>] [-f <directory-path>] [-u <user>] [-pass<password>]\nServer:\tcsyc [-s] [-p <port>] [-cc <packets>]\n";

struct arg {
	mode type = none;
	unsigned int port = DEFAULT_PORT;
	std::string host = ""; 
	std::string dir = "";
	std::string user = "";
	std::string password = "";
	unsigned int maxPPS = DEFAULT_MAX_PPS;
};

struct arg* parseParameter(int argc, char* argv[]);
void launchServer(unsigned int port, unsigned int maxPPS);
void launchClient(unsigned int port, std::string host, std::string dir, std::string userName, std::string password);
void printClientHelp();
int main(int argc, char* argv[]);
