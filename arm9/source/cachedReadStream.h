#ifndef CACHEDSTREAM_H
#define CACHEDSTREAM_H

#include "binaryStream.h"

#include <memory>

class CachedReadStream : public BinaryReadStream {
public:
	CachedReadStream(BinaryReadStream *realStream, bool ownsStream, size_t maxCacheSize = 0);
	CachedReadStream(std::unique_ptr<BinaryReadStream> &&realStream, size_t maxCacheSize = 0);
	~CachedReadStream() override;

	void setPos(size_t newPos) override;
	size_t getPos() const noexcept override;
	size_t read(uint8_t *buf, size_t size) override;
	size_t getLength() const noexcept override;

	void invalidateCache();

private:
	BinaryReadStream *const realStream;
	const bool ownsStream;

	size_t virtualPos;

	size_t readFromRealStream(uint8_t *buf, size_t size, size_t pos);

	void setupCache();

	void cacheBlock(size_t startPos);

	void *cache;
	size_t cacheStart;
	size_t cacheCurSize;
	size_t cacheMaxSize;
};

#endif
