#include "binaryStream.h"

BinaryReadStream::BinaryReadStream() {}
BinaryReadStream::~BinaryReadStream() {}

BinaryWriteStream::BinaryWriteStream() {}
BinaryWriteStream::~BinaryWriteStream() {}

ManualPosBinaryReadStream::ManualPosBinaryReadStream() : position(0) {}
void ManualPosBinaryReadStream::setPos(size_t newPos) { position = newPos; }
size_t ManualPosBinaryReadStream::getPos() const { return position; }

ManualPosBinaryWriteStream::ManualPosBinaryWriteStream() : position(0) {}
void ManualPosBinaryWriteStream::setPos(size_t newPos) { position = newPos; }
size_t ManualPosBinaryWriteStream::getPos() const { return position; }