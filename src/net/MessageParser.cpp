#include "MessageParser.h"

using namespace std;
using namespace net;

void parseMessage(unsigned char readByte, int socketFD, AbstractMessage* result) {

	// Get message type:
	unsigned char msgType = readByte >> 4;

	switch(msgType) {
		case 1:
			cout << "Received ClientHelloMessage." << endl;
			result = new ClientHelloMessage(0);
			break;

		default:
			cout << "Received unknown message type: " << msgType << endl;
			result = NULL;
			break;
	}
}