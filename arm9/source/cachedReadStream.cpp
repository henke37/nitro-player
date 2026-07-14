#include "cachedReadStream.h"

#include <cassert>
#include <cstring>
#include <algorithm>

CachedReadStream::CachedReadStream(BinaryReadStream *realStream, bool ownsStream, size_t maxCacheSize) : realStream(realStream), ownsStream(ownsStream), virtualPos(0), cache(nullptr), cacheStart(0), cacheCurSize(0), cacheMaxSize(maxCacheSize) {
	assert(this->realStream);

	setupCache();
}
CachedReadStream::CachedReadStream(std::unique_ptr<BinaryReadStream> &&realStream, size_t maxCacheSize) : realStream(realStream.release()), ownsStream(true), virtualPos(0), cache(nullptr), cacheStart(0), cacheCurSize(0), cacheMaxSize(maxCacheSize) {
	assert(this->realStream);

	setupCache();
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

	if(cacheMaxSize == 0) {
		return;
	}

	cache = malloc(cacheMaxSize);
	cacheCurSize = 0;

	assert(cache);

	cacheBlock(0);
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

	if(!cache) {
		size_t readSize = readFromRealStream(buf, size, virtualPos);
		virtualPos += readSize;
		return readSize;
	}

	size_t readProgress = 0;

	//was there anything before the cache that we needed to read?

	if(virtualPos < cacheStart) {
		size_t toReadFromRealStream = std::min(size, cacheStart - virtualPos);
		size_t readSize = readFromRealStream(buf, toReadFromRealStream, virtualPos);

		readProgress += readSize;
		virtualPos += readSize;

		if(readSize < toReadFromRealStream) {
			invalidateCache();
			return readProgress;
		}
	}


readFromCache:
	if(readProgress == size) {
		return readProgress;
	}


	assert(virtualPos >= cacheStart);

	size_t cacheEnd = cacheStart + cacheCurSize;

	if(virtualPos < cacheEnd) {
		assert(size > readProgress);
		size_t cacheOffset = virtualPos - cacheStart;
		size_t cacheRemaining = cacheCurSize - cacheOffset;
		size_t readSize = std::min(size - readProgress, cacheRemaining);

		memcpy(buf + readProgress, (uint8_t *)cache + cacheOffset, readSize);
		readProgress += readSize;
		virtualPos += readSize;
	}

	size_t remainingSize = size - readProgress;

	//cache read finished progress, we are done here
	if(remainingSize == 0) {
		return readProgress;
	}

	if(remainingSize > cacheMaxSize) {
		//if we have more to read than the cache can hold, just read it directly
		size_t toCache = remainingSize % cacheMaxSize;
		assert(remainingSize >= toCache);
		size_t readSize = remainingSize - toCache;

		size_t readFromRealStreamSize = readFromRealStream(buf + readProgress, readSize, virtualPos);

		readProgress += readFromRealStreamSize;
		assert(readProgress <= size);
		virtualPos += readFromRealStreamSize;

		if(readFromRealStreamSize < readSize) {
			return readProgress;
		}
	}

	cacheBlock(virtualPos);

	if(cacheCurSize == 0) {
		return readProgress;
	}

	goto readFromCache;
}