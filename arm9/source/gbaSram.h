#ifndef GBASRAM_H
#define GBASRAM_H

#include "memoryStream.h"

class GbaSramReadStream : public BaseMemoryReadStream {
public:
	GbaSramReadStream(uint8_t *baseAddr, size_t length);
	~GbaSramReadStream();

	size_t read(uint8_t *buff, size_t size) override;
private:
};

class GbaSramWriteStream : public BaseMemoryWriteStream {
public:
	GbaSramWriteStream(uint8_t *baseAddr, size_t length);
	~GbaSramWriteStream();

	size_t write(const uint8_t *buff, size_t size) override;
private:
};

class GbaEEPromReadStream : public BinaryReadStream {
public:
	GbaEEPromReadStream();
	~GbaEEPromReadStream();
};

class GbaEEPromWriteStream : public BinaryWriteStream {
	GbaEEPromWriteStream();
	~GbaEEPromWriteStream();
};

class GbaFlashReadStream : public BinaryReadStream {
public:
	GbaFlashReadStream();
	~GbaFlashReadStream();
};

class GbaFlashWriteStream : public BinaryWriteStream {
	GbaFlashWriteStream();
	~GbaFlashWriteStream();
};

#endif