#pragma once

namespace net {
	class AbstractMessage {
	public:
		virtual void toByteArray() {};

		AbstractMessage(char type);
		char getType();

	private:
		char type;
	};
}