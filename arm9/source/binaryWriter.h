#ifndef BINARY_WRITER_H
#define BINARY_WRITER_H

#include "binaryStream.h"

#include <memory>
#include <string>

class BinaryWriter {
public:
	BinaryWriter(BinaryWriteStream *stream, bool ownsStream);
	BinaryWriter(std::unique_ptr<BinaryWriteStream> &&stream);
	~BinaryWriter();

	BinaryWriter &operator=(BinaryWriter &&old);
	BinaryWriter(BinaryWriter &&old);


	void writeBytes(const uint8_t *buf, size_t len);

	void writeByte(uint8_t val);
	void writeSignedByte(int8_t val);
	void writeLEShort(uint16_t val);
	void writeBEShort(uint16_t val);
	void writeLESignedShort(int16_t val);
	void writeBESingedShort(int16_t val);
	void writeLELong(uint32_t val);
	void writeBELong(uint32_t val);
	void writeLESingedLong(int32_t val);
	void writeBESingedLong(int32_t val);

	void writeString(const std::string &str);
	void writeStringWithByteLen(const std::string &str);
	void writeZeroTermString(const std::string &str);
	void writeLine(const std::string &str);
	void writeLEUTF16String(const std::u16string &str);
	void writeZeroTermLEUTF16String(const std::u16string &str);
	void writeLEUTF16Line(const std::u16string &str);

	bool isAtEnd() const;

	void skip(size_t len);
	void setPos(size_t newPos);
	size_t getPos() const;

private:
	BinaryWriteStream *stream;
	bool ownsStream;
};

#endif