#include "MessageParser.h"

using namespace std;
using namespace net;

AbstractMessage* parseMessage(unsigned char readByte, int socketFD) {

	// Get message type:
	unsigned char msgType = readByte >> 4;

	switch(msgType) {
		case 1:
			cout << "Received ClientHelloMessage." << endl;
			return new ClientHelloMessage(0);

		default:
			cout << "Received unknown message type: " << msgType << endl;
			return NULL;
	}
}