#pragma once

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
		void calcChecksum(char* buffer, int& bufferLength, unsigned int startIndex);

	private:
		char type;
	};
}