#include "binaryReader.h"

#include <cassert>

BinaryReader::BinaryReader(BinaryReadStream *stream, bool ownsStream) : stream(stream), ownsStream(ownsStream) {
	assert(stream);
}

BinaryReader::BinaryReader(std::unique_ptr<BinaryReadStream> &&stream) : stream(stream.release()), ownsStream(true) {
	assert(this->stream);
}

BinaryReader::~BinaryReader() {
	if(ownsStream) {
		delete stream;
	}
}

void BinaryReader::readBytes(uint8_t *buf, size_t len) {
	size_t readSize = stream->read(buf, len);
	assert(readSize==len);
}

void BinaryReader::skip(size_t len) {
	stream->setPos(stream->getPos()+len);
}

void BinaryReader::setPos(size_t newPos) {
	stream->setPos(newPos);
}

size_t BinaryReader::getPos() const {
	return stream->getPos();
}

bool BinaryReader::isAtEnd() const {
	return stream->getPos() == stream->getLength();
}

uint8_t BinaryReader::readByte() {
	uint8_t buf;
	size_t readSize = stream->read(&buf, 1);
	assert(readSize==1);
	
	return buf;
}

int8_t BinaryReader::readSignedByte() {
	return (int8_t)readByte();
}

uint16_t BinaryReader::readLEShort() {
	uint8_t buf[2];
	
	size_t readSize = stream->read(buf, 2);
	assert(readSize==2);
	
	return buf[0] | (buf[1] << 8);
}

int16_t BinaryReader::readLESignedShort() {
	return (int16_t)readLEShort();
}

uint32_t BinaryReader::readLE24Bit() {
	uint8_t buf[3];

	size_t readSize = stream->read(buf, 3);
	assert(readSize == 3);

	return buf[0] | (buf[1] << 8) | (buf[2] << 16);
}

uint32_t BinaryReader::readLELong() {
	uint8_t buf[4];
	
	size_t readSize = stream->read(buf, 4);
	assert(readSize==4);
	
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

int32_t BinaryReader::readLESingedLong() {
	return (int32_t)readLELong();
}

std::string BinaryReader::readString(size_t len) {
	char *buf=new char[len];
	
	size_t readSize = stream->read((uint8_t*)buf, len);
	assert(readSize==len);
	
	std::string str(buf, readSize);
	
	delete[] buf;
	
	return str;
}

std::string BinaryReader::readStringWithByteLen() {
	size_t len = readByte();
	return readString(len);
}

std::u16string BinaryReader::readLEUTF16String(size_t len) {
	char16_t *buf=new char16_t[len];
	
	size_t readSize = stream->read((uint8_t*)buf, len*2);
	readSize/=2;
	assert(readSize==len);
	
	std::u16string str(buf, readSize);
	
	delete[] buf;
	
	return str;
}

std::string BinaryReader::readZeroTermString() {
	std::string str;
	
	for(;;) {
		uint8_t b=readByte();
		if(!b) break;
		
		str.append(1, (char) b);
	}
	
	return str;
}

std::string BinaryReader::readLine() {
	std::string str;
	
	for(;!isAtEnd();) {
		uint8_t b=readByte();
		if(b=='\r') {
			b=readByte();
			assert(b=='\n');
			break;
		} else if(b=='\n') {
			assert(0);
		}
		
		str.append(1, (char) b);
	}
	
	return str;
}

std::u16string BinaryReader::readLEUTF16Line() {
	std::u16string str;
	
	for(;!isAtEnd();) {
		char16_t b=readLEShort();
		if(b==u'\r') {
			b=readLEShort();
			assert(b==u'\n');
			break;
		} else if(b==u'\n') {
			assert(0);
		}
		
		str.append(1, (char16_t) b);
	}
	
	return str;
}