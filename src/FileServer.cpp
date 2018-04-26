#include "FileServer.h"

using namespace net;
using namespace std;

FileServer::FileServer(unsigned short port) {
	this->port = port;
	this->state = stopped;
}

void FileServer::start() {
	if(state != stopped) {
		cerr << "Unable to start file server! State != stopped" << endl;
	}
	else {
		state = starting;
		server = Server(port);
		server.start();
		state = running;
	}
}

void FileServer::stop() {

}