#include "gbaSram.h"

#include <nds.h>

#include <cstring>

GbaSramReadStream::GbaSramReadStream(uint8_t *baseAddr, size_t length) : BaseMemoryReadStream(baseAddr, length) {
}

GbaSramReadStream::~GbaSramReadStream() {}

GbaSramWriteStream::GbaSramWriteStream(uint8_t *baseAddr, size_t length) : BaseMemoryWriteStream(baseAddr, length) {}

GbaSramWriteStream::~GbaSramWriteStream() {}

