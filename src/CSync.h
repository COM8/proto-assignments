#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include "Helpers.h"
#include "Filesystem.h"
#include "FileClient2.h"
#include "FileServer2.h"
#include "net/ClientHelloMessage.h"

const int DEFAULT_PORT = 1234;
enum mode { server, client, none };

const std::string helpString = "Client:\tcsync [-h <hostname|ip-addr>] [-p <port>] [-f <directory-path>]\nServer:\tcsyc [-s] [-p <port>]\n";

struct arg {
	mode type = none;
	unsigned int port = DEFAULT_PORT;
	std::string host = "", dir = "";
};

struct arg* parseParameter(int argc, char* argv[]);
void launchServer(unsigned int port);
void launchClient(unsigned int port, std::string host, std::string dir);
void printClientHelp();
int main(int argc, char* argv[]);
