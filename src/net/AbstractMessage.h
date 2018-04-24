#pragma once
#include <iostream>
#include <bitset>

namespace net {

	struct Message
	{
		int bufferLength;
		char* buffer;
	};

	class AbstractMessage {
	public:
		virtual void createBuffer(struct Message* msg) { msg->bufferLength = -1; };

		AbstractMessage(char type);
		char getType();
		void addChecksum(struct Message* msg, int bitOffset);
		void printMessage(struct Message* msg);

	protected:
		char type;

		void setBufferValue(struct Message* msg, char* value, int valueLength, int bitOffset);
		void printByteArray(const char* c, int length);
		void printByte(char c);

	private:
		void setByteWithOffset(struct Message* msg, char value, int bitOffset);
	};
}