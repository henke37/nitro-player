#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

#include "binaryStream.h"

class BaseMemoryReadStream : public ManualPosBinaryReadStream {
public:
	~BaseMemoryReadStream();
	size_t read(uint8_t *buff, size_t size) override;
	size_t getLength() const noexcept override;
protected:
	BaseMemoryReadStream(uint8_t *baseAddr, size_t length);
	uint8_t *baseAddr;
	size_t length;
};

class BaseMemoryWriteStream : public ManualPosBinaryWriteStream {
public:
	~BaseMemoryWriteStream();

	size_t write(const uint8_t *buff, size_t size) override;
	size_t getLength() const noexcept override;
protected:
	BaseMemoryWriteStream(uint8_t *baseAddr, size_t length);
	uint8_t *baseAddr;
	size_t length;
};

#endif