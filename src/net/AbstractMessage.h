#pragma once

#include <iostream>

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
		AbstractMessage() = default;
		char getType();
		void addChecksum(struct Message* msg, int bitOffset);

	protected:
		char type;

		static unsigned char* getBytesWithOffset(unsigned char* buffer, int bitOffset, int bitLength);
		void setBufferValue(struct Message* msg, char* value, int valueLength, int bitOffset);

	private:
		void setByteWithOffset(struct Message* msg, char value, int bitOffset);
		static unsigned char getByteWithOffset(unsigned char* buffer, int bitOffset);
	};
}