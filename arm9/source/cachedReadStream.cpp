#include "cachedReadStream.h"

#include <cassert>
#include <cstring>

CachedReadStream::CachedReadStream(BinaryReadStream *realStream, bool ownsStream, size_t maxCacheSize) : realStream(realStream), ownsStream(ownsStream), virtualPos(0), cache(nullptr), cacheStart(0), cacheMaxSize(maxCacheSize) {
	assert(this->realStream);

	setupCache();

	if(maxCacheSize == 0) {
		cacheBlock(0);
	}
}
CachedReadStream::CachedReadStream(std::unique_ptr<BinaryReadStream> &&realStream, size_t maxCacheSize) : realStream(realStream.release()), ownsStream(true), virtualPos(0), cache(nullptr), cacheStart(0), cacheMaxSize(maxCacheSize) {
	assert(this->realStream);

	setupCache();

	if(maxCacheSize == 0) {
		cacheBlock(0);
	}
}
CachedReadStream::~CachedReadStream() {
	if(ownsStream) {
		delete realStream;
	}
	free(cache);
}

size_t CachedReadStream::getLength() const noexcept {
	return realStream->getLength();
}

void CachedReadStream::invalidateCache() {
	free(cache);
	cache = nullptr;
	cacheStart = 0;
	cacheCurSize = 0;
}

size_t CachedReadStream::readFromRealStream(uint8_t *buf, size_t size, size_t pos) {
	assert(realStream);

	realStream->setPos(pos);
	size_t readSize = realStream->read(buf, size);
	return readSize;
}

void CachedReadStream::setupCache() {
	size_t realLen = realStream->getLength();
	if(cacheMaxSize == 0) {
		cacheMaxSize = realLen;
	} else if(cacheMaxSize > realLen) {
		cacheMaxSize = realLen;
	}

	cache = malloc(cacheMaxSize);
}

void CachedReadStream::cacheBlock(size_t startPos) {
	assert(cache);

	//simplification: no need to reuse old data, hoping for an overlap

	cacheStart = startPos;

	size_t readSize = readFromRealStream((uint8_t *)cache, cacheMaxSize, startPos);
	cacheCurSize = readSize;
}

void CachedReadStream::setPos(size_t newPos) {
	virtualPos = newPos;
}
size_t CachedReadStream::getPos() const noexcept {
	return virtualPos;
}

size_t CachedReadStream::read(uint8_t *buf, size_t size) {

	size_t readLen = 0;

	//first read what already exists in the cache

	if(virtualPos >= cacheStart && virtualPos < cacheStart + cacheCurSize) {
		size_t cacheOffset = virtualPos - cacheStart;
		size_t toReadFromCache = std::min(size, cacheCurSize - cacheOffset);
		memcpy(buf, (uint8_t *)cache + cacheOffset, toReadFromCache);

		readLen += toReadFromCache;
	}

	//was there anything before the cache that we needed to read?

	if(virtualPos < cacheStart) {
		size_t toReadFromRealStream = std::min(size, cacheStart - virtualPos);
		size_t readSize = readFromRealStream(buf, toReadFromRealStream, virtualPos);
		assert(readSize == toReadFromRealStream);

		readLen += readSize;
	}

	//if there was anything after the cache, we read it into the cache and then read from the cache

	if(virtualPos + size > cacheStart + cacheCurSize) {
		cacheBlock(virtualPos);
		size_t cacheOffset = virtualPos - cacheStart;
		size_t toReadFromCache = std::min(size, cacheCurSize - cacheOffset);
		memcpy(buf, (uint8_t *)cache + cacheOffset, toReadFromCache);

		readLen += toReadFromCache;
	}

	virtualPos += readLen;

	return readLen;
}