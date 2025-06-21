#include "substream.h"

#include <cassert>

SubStream::SubStream(BinaryReadStream *realStream, size_t offset, size_t limit, bool ownsStream) : realStream(realStream), offset(offset), limit(limit), ownsStream(ownsStream), virtualPos(0) {
	assert(this->realStream);
}
SubStream::SubStream(std::unique_ptr<BinaryReadStream> &&realStream, size_t offset, size_t limit) : realStream(realStream.release()), offset(offset), limit(limit), ownsStream(true), virtualPos(0) {
	assert(this->realStream);
}
SubStream::~SubStream() {
	if(ownsStream) {
		delete realStream;
	}
}

void SubStream::applyPos() {
	realStream->setPos(offset+virtualPos);
}

void SubStream::setPos(size_t newPos) {
	virtualPos=newPos;
	applyPos();
}
size_t SubStream::getPos() const {
	return virtualPos;
}
size_t SubStream::read(uint8_t *buf, size_t size) {
	applyPos();
	
	size_t remaining = getLength()-virtualPos;
	if(size > remaining) {
		size=remaining;
	}
	
	size_t readSize=realStream->read(buf, size);
	
	if(readSize<0) return readSize;
	
	virtualPos+=readSize;
	
	return readSize;
}
size_t SubStream::getLength() const {
	size_t realLen=realStream->getLength();
	if(realLen-offset>limit) return limit;
	
	return realLen-offset;
}