#include "memoryStream.h"

#include <cstring>

BaseMemoryReadStream::BaseMemoryReadStream(uint8_t *baseAddr, size_t length) : baseAddr(baseAddr), length(length) {}
BaseMemoryReadStream::~BaseMemoryReadStream() {}

size_t BaseMemoryReadStream::read(uint8_t *buff, size_t size) {
	size_t bytesLeft = length - position;
	size_t readCnt = size <= bytesLeft ? size: bytesLeft;

	uint8_t *readPtr = baseAddr + position;

	std::memcpy(buff, readPtr, readCnt);

	position += readCnt;

	return readCnt;
}

size_t BaseMemoryReadStream::getLength() const {
	return length;
}


BaseMemoryWriteStream::BaseMemoryWriteStream(uint8_t *baseAddr, size_t length) : baseAddr(baseAddr), length(length) {}

BaseMemoryWriteStream::~BaseMemoryWriteStream() {}

size_t BaseMemoryWriteStream::write(const uint8_t *buff, size_t size) {
	size_t bytesLeft = length - position;
	size_t writeCnt = size <= bytesLeft ? size : bytesLeft;

	uint8_t *writePtr = baseAddr + position;

	std::memcpy(writePtr, buff, writeCnt);

	position += writeCnt;

	return writeCnt;
}

size_t BaseMemoryWriteStream::getLength() const {
	return length;
}

