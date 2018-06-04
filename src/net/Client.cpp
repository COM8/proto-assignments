#include "net/Client.h"

using namespace std;
using namespace net;
using namespace sec;

Client::Client(string hostAddr, unsigned short port, DiffieHellman *enc)
{
	this->hostAddr = hostAddr;
	this->enc = enc;
	this->port = port;

	this->sockFD = -1;
	srand(time(NULL));
	init();
}

void Client::init()
{
	if ((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "UDP socket creation failed!" << endl;
	}
	else
	{
		serverAddressStruct = {};
		serverAddressStruct.sin_family = AF_INET;
		serverAddressStruct.sin_port = htons(port);

		// Look up the address of the server given its name:
		struct hostent *hp = {};
		hp = gethostbyname(hostAddr.c_str());
		if (!hp)
		{
			cerr << "UDP client unable to look up host: " << hostAddr << endl;
		}
		else
		{
			// Add host address to the address structure:
			memcpy((void *)&serverAddressStruct.sin_addr, hp->h_addr_list[0], hp->h_length);
		}
	}
}

bool Client::send(AbstractMessage *msg)
{
	Message msgStruct = {};
	msg->createBuffer(&msgStruct);
	bool result = send(&msgStruct);
	delete[] msgStruct.buffer;
	return result;
}

bool Client::send(Message *msg)
{
	// Print message:
	// AbstractMessage::printMessage(&msgStruct);

	bool success = true;

	// Message drop chance:
	if (rand() % 100 + 1 <= MESSAGE_DROP_CHANCE)
	{
		cout << "Droped message!" << endl;
	}
	else
	{
		// Encrypt message:
		if(enc && enc->isConnectionSecure()) {
			enc->Encrypt(msg->buffer, msg->bufferLength);
		}

		if (sendto(sockFD, msg->buffer, msg->bufferLength, 0, (struct sockaddr *)&serverAddressStruct, sizeof(serverAddressStruct)) < 0)
		{
			cerr << "UDP client failed to send message to: " << hostAddr << " and port: " << port << endl;
			success = false;
		}
	}
	return success;
}