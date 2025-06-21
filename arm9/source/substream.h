#ifndef SUBSTREAM_H
#define SUBSTREAM_H

#include "binaryStream.h"

#include <memory>

class SubStream : public BinaryReadStream {
public:
	SubStream(BinaryReadStream *realStream, size_t offset, size_t limit, bool ownsStream);
	SubStream(std::unique_ptr<BinaryReadStream> &&realStream, size_t offset, size_t limit);
	~SubStream();
	
	SubStream(SubStream &&old);
	SubStream &operator=(SubStream &&old);
	SubStream(const SubStream *old, size_t offset, size_t limit, bool ownsStream);
	
	virtual void setPos(size_t newPos);
	virtual size_t getPos() const;
	size_t read(uint8_t *buf, size_t size);
	size_t getLength() const;

private:
	BinaryReadStream *realStream;
	size_t offset;
	size_t limit;
	bool ownsStream;
	
	size_t virtualPos;
	
	void applyPos();
};

#endif