#include "FileServer.h"

using namespace net;
using namespace std;

FileServer::FileServer(unsigned short port) : port(port), state(stopped), shouldConsumerRun(false) {
	cpQueue = new Queue<net::AbstractMessage>();
}

void FileServer::start() {
	if(state != stopped) {
		cerr << "Unable to start file server! State != stopped" << endl;
	}
	else {
		state = starting;
		server = Server(port, cpQueue);
		server.start();
		startConsumerThread();
		state = running;
	}
}

void FileServer::startConsumerThread() {
	shouldConsumerRun = true;
	consumerThread = new thread(&FileServer::consumerTask, this);
}

void FileServer::stopConsumerThread() {
	shouldConsumerRun = false;
	if(consumerThread && consumerThread->joinable()) {
		consumerThread->join();
	}
}

void FileServer::consumerTask() {
	while(shouldConsumerRun) {
		cpQueue->pop();
	}
}

void FileServer::stop() {
	stopConsumerThread();
}