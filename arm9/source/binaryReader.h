#ifndef BINARY_READER_H
#define BINARY_READER_H

#include "binaryStream.h"

#include <string>
#include <memory>

class BinaryReader {
public:
	BinaryReader(BinaryReadStream *stream, bool ownsStream);
	BinaryReader(std::unique_ptr<BinaryReadStream> &&stream);
	~BinaryReader();
	
	BinaryReader &operator=(BinaryReader &&old);
	BinaryReader(BinaryReader &&old);
	
	void readBytes(uint8_t *buf, size_t len);
	
	uint8_t readByte();
	int8_t readSignedByte();
	uint16_t readLEShort();
	uint16_t readBEShort();
	int16_t readLESignedShort();
	int16_t readBESingedShort();
	uint32_t readLELong();
	uint32_t readBELong();
	int32_t readLESingedLong();
	int32_t readBESingedLong();
	
	std::string readString(size_t len);
	std::string readStringWithByteLen();
	std::string readZeroTermString();
	std::string readLine();
	std::u16string readLEUTF16String(size_t len);
	std::u16string readZeroTermLEUTF16String();
	std::u16string readLEUTF16Line();
	
	bool isAtEnd() const;
	
	void skip(size_t len);
	void setPos(size_t newPos);
	size_t getPos() const;
	
private:
	BinaryReadStream *stream;
	bool ownsStream;
};

#endif