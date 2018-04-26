#include "Helpers.h"

using namespace std;
using namespace net;

void printMessage(struct Message* msg) {
	printByteArray(msg->buffer, msg->bufferLength);
}

void printByteArray(const char* c, int length) {
	for (int i = 0; i < length; ++i)
	{
		printByte(c[i]);
	}
	cout << endl;
}

void printByte(char c) {
	cout << bitset<8>(c) << ' ';
}