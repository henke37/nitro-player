#include "binaryWriter.h"

#include <cassert>

BinaryWriter::BinaryWriter(BinaryWriteStream *stream, bool ownsStream) : stream(stream), ownsStream(ownsStream) {
	assert(stream);
}

BinaryWriter::BinaryWriter(std::unique_ptr<BinaryWriteStream> &&stream) : stream(stream.release()), ownsStream(true) {
	assert(this->stream);
}

BinaryWriter::~BinaryWriter() {
	if(ownsStream) {
		delete stream;
	}
}

void BinaryWriter::skip(size_t len) {
	stream->setPos(stream->getPos() + len);
}

void BinaryWriter::setPos(size_t newPos) {
	stream->setPos(newPos);
}

size_t BinaryWriter::getPos() const {
	return stream->getPos();
}

bool BinaryWriter::isAtEnd() const {
	return stream->getPos() == stream->getLength();
}

void BinaryWriter::writeBytes(const uint8_t *buf, size_t len) {
	size_t written = stream->write(buf, len);
	assert(written == len);
}

#define writeVal(T) \
	size_t written = stream->write(reinterpret_cast<const uint8_t *>(&val), sizeof(T)); \
	assert(written == sizeof(T))

void BinaryWriter::writeByte(uint8_t val) {
	writeVal(uint8_t);
}

void BinaryWriter::writeSignedByte(int8_t val) {
	writeVal(int8_t);
}

void BinaryWriter::writeLEShort(uint16_t val) {
	writeVal(uint16_t);
}

void BinaryWriter::writeLESignedShort(int16_t val) {
	writeVal(int16_t);
}

void BinaryWriter::writeLELong(uint32_t val) {
	writeVal(uint32_t);
}

void BinaryWriter::writeLESingedLong(int32_t val) {
	writeVal(int32_t);
}

#undef writeVal

void BinaryWriter::writeString(const std::string &str) {
	size_t written = stream->write(reinterpret_cast<const uint8_t *>(str.c_str()), str.size());
	assert(written == str.size());
}

void BinaryWriter::writeStringWithByteLen(const std::string &str) {
	size_t len = str.size();
	assert(len <= 255);
	writeByte(len);
	writeString(str);
}

void BinaryWriter::writeZeroTermString(const std::string &str) {
	writeString(str);
	writeByte(0);
}

void BinaryWriter::writeLine(const std::string &str) {
	writeString(str);
	writeByte('\r');
	writeByte('\n');
}

void BinaryWriter::writeLEUTF16String(const std::u16string &str) {
	size_t written = stream->write(reinterpret_cast<const uint8_t *>(str.c_str()), str.size()*sizeof(char16_t));
	assert(written == str.size() * sizeof(char16_t));
}

void BinaryWriter::writeZeroTermLEUTF16String(const std::u16string &str) {
	writeLEUTF16String(str);
	writeLEShort(0);
}

void BinaryWriter::writeLEUTF16Line(const std::u16string &str) {
	writeLEUTF16String(str);
	writeLEShort('\r');
	writeLEShort('\n');
}
