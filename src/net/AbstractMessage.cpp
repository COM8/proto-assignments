#include "net/AbstractMessage.h"

using namespace net;
using namespace std;

AbstractMessage::AbstractMessage(unsigned char type)
{
	this->type = type;
}

AbstractMessage::~AbstractMessage()
{
}

unsigned char AbstractMessage::getType()
{
	return type;
}

unsigned int AbstractMessage::caclCRC32(unsigned char *buffer, int bufferLength)
{
	CRC32 crc = CRC32();
	crc.add(buffer, bufferLength);
	unsigned char crc32Bytes[CRC32::HashBytes];
	crc.getHash(crc32Bytes);
	return getUnsignedIntFromMessage(crc32Bytes, 0);
}

void AbstractMessage::addChecksum(struct Message *msg, unsigned int checkSumOffsetBits)
{
	// Ensure the CRC32 field is filled with 0:
	setBufferUnsignedInt(msg, 0, checkSumOffsetBits);

	unsigned int crc32 = caclCRC32(msg->buffer, msg->bufferLength);
	setBufferUnsignedInt(msg, crc32, checkSumOffsetBits);
}

bool AbstractMessage::isChecksumValid(struct ReadMessage *msg, unsigned int checkSumOffsetBits)
{
	unsigned int crc32Read = getUnsignedIntFromMessage(msg->buffer, checkSumOffsetBits);

	// Fill the CRC32 field with 0:
	setBufferUnsignedInt(msg->buffer, 0, checkSumOffsetBits);

	// Clac new CRC32:
	unsigned int crc32Calc = caclCRC32(msg->buffer, msg->bufferLength);

	if (crc32Calc != crc32Read)
	{
		cout << "Invalid CRC32! Should: " << crc32Calc << ", Read: " << crc32Read << ", BufLength: " << msg->bufferLength << ", BitsOffset: " << checkSumOffsetBits << " Type: " << ((msg->buffer[0] & 0xf0) >> 4) << endl;
	}

	return crc32Read == crc32Calc;
}

void AbstractMessage::printMessage(struct Message *msg)
{
	AbstractMessage::printByteArray(msg->buffer, msg->bufferLength);
}

void AbstractMessage::printByteArray(unsigned char *c, int length)
{
	for (int i = 0; i < length; ++i)
	{
		AbstractMessage::printByte(c[i]);
	}
	cout << endl;
}

void AbstractMessage::printByte(unsigned char c)
{
	cout << bitset<8>(c) << ' ';
}

void AbstractMessage::setBufferValue(struct Message *msg, unsigned char *value, int valueLength, int bitOffset)
{
	AbstractMessage::setBufferValue(msg->buffer, value, valueLength, bitOffset);
}

void AbstractMessage::setBufferValue(unsigned char *buffer, unsigned char *value, int valueLength, int bitOffset)
{
	for (int i = 0; i < valueLength; ++i)
	{
		AbstractMessage::setByteWithOffset(buffer, value[i], bitOffset + (8 * i));
	}
}

void AbstractMessage::setByteWithOffset(struct Message *msg, unsigned char value, int bitOffset)
{
	AbstractMessage::setByteWithOffset(msg->buffer, value, bitOffset);
}

void AbstractMessage::setByteWithOffset(unsigned char *buffer, unsigned char value, int bitOffset)
{
	int byteIndex = bitOffset / 8;
	int bitOffsetMod = bitOffset % 8;

	if (bitOffsetMod == 0)
	{
		buffer[byteIndex] = value;
	}
	else
	{
		// Reset bits:
		buffer[byteIndex] &= (unsigned char)0xff << (8 - bitOffsetMod);
		buffer[byteIndex + 1] &= (unsigned char)0xff >> bitOffsetMod;

		// Set bits:
		buffer[byteIndex] |= (value >> bitOffsetMod);
		buffer[byteIndex + 1] |= value << (8 - bitOffsetMod);
	}
}

void AbstractMessage::setBufferUnsignedInt(struct Message *msg, unsigned int i, int bitOffset)
{
	AbstractMessage::setBufferUnsignedInt(msg->buffer, i, bitOffset);
}

void AbstractMessage::setBufferUnsignedInt(unsigned char *buffer, unsigned int i, int bitOffset)
{
	unsigned char intArray[4];
	for (int e = 0; e < 4; e++)
	{
		intArray[3 - e] = (i >> (e * 8));
	}
	AbstractMessage::setBufferValue(buffer, intArray, 4, bitOffset);
}

void AbstractMessage::setBufferUnsignedShort(struct Message *msg, unsigned short i, int bitOffset)
{
	AbstractMessage::setBufferUnsignedShort(msg->buffer, i, bitOffset);
}

void AbstractMessage::setBufferUnsignedShort(unsigned char *buffer, unsigned short i, int bitOffset)
{
	unsigned char shortArray[2];
	for (int e = 0; e < 2; e++)
	{
		shortArray[1 - e] = (i >> (e * 8));
	}
	AbstractMessage::setBufferValue(buffer, shortArray, 2, bitOffset);
}

void AbstractMessage::setBufferUint64_t(struct Message *msg, uint64_t i, int bitOffset)
{
	AbstractMessage::setBufferUint64_t(msg->buffer, i, bitOffset);
}

void AbstractMessage::setBufferUint64_t(unsigned char *buffer, uint64_t i, int bitOffset)
{
	unsigned char intArray[8];
	for (int e = 0; e < 8; e++)
	{
		intArray[7 - e] = (i >> (e * 8));
	}
	AbstractMessage::setBufferValue(buffer, intArray, 8, bitOffset);
}

unsigned char *AbstractMessage::getBytesWithOffset(unsigned char *buffer, int bitOffset, int bitLength)
{
	return getBytesWithOffset(buffer, bitOffset, (uint64_t)bitLength);
}

unsigned char *AbstractMessage::getBytesWithOffset(unsigned char *buffer, int bitOffset, uint64_t bitLength)
{
	uint64_t lengthBytes = bitLength / 8;
	unsigned char *result = new unsigned char[lengthBytes];
	for (uint64_t i = 0; i < lengthBytes; i++)
	{
		result[i] = getByteWithOffset(buffer, bitOffset + i * 8);
	}

	int lengthMod = bitLength % 8;
	if (lengthMod != 0)
	{
		result[lengthBytes - 1] &= (unsigned char)0xff << (8 - lengthMod);
	}

	return result;
}

unsigned char AbstractMessage::getByteWithOffset(unsigned char *buffer, int bitOffset)
{
	int bitOffsetMod = bitOffset % 8;
	int byteOffset = bitOffset / 8;
	if (bitOffsetMod == 0)
	{
		return buffer[byteOffset];
	}
	else
	{
		return (buffer[byteOffset] << bitOffsetMod) | ((unsigned char)(buffer[byteOffset + 1]) >> bitOffsetMod);
	}
}

unsigned int AbstractMessage::getUnsignedIntFromMessage(unsigned char *buffer, int bitOffset)
{
	unsigned char *intArray = AbstractMessage::getBytesWithOffset(buffer, bitOffset, 32);
	unsigned int val = static_cast<unsigned int>(intArray[0]) << 24 | intArray[1] << 16 | intArray[2] << 8 | intArray[3];
	delete[] intArray;
	return val;
}

unsigned int AbstractMessage::getUnsignedShortFromMessage(unsigned char *buffer, int bitOffset)
{
	unsigned char *shortArray = AbstractMessage::getBytesWithOffset(buffer, bitOffset, 32);
	unsigned short val = static_cast<unsigned short>(shortArray[0]) << 8 | shortArray[1];
	delete[] shortArray;
	return val;
}

uint64_t AbstractMessage::getUnsignedInt64FromMessage(unsigned char *buffer, int bitOffset)
{
	unsigned char *int64Array = AbstractMessage::getBytesWithOffset(buffer, bitOffset, 64);
	uint64_t val = static_cast<uint64_t>(int64Array[0]) << 56 | static_cast<uint64_t>(int64Array[1]) << 48 | static_cast<uint64_t>(int64Array[2]) << 40 | static_cast<uint64_t>(int64Array[3]) << 32 | static_cast<uint64_t>(int64Array[4]) << 24 | static_cast<uint64_t>(int64Array[5]) << 16 | static_cast<uint64_t>(int64Array[6]) << 8 | static_cast<uint64_t>(int64Array[7]);
	delete[] int64Array;
	return val;
}