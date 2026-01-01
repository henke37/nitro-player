#ifndef BINARY_STREAM_H
#define BINARY_STREAM_H

#include <cstddef>
#include <cstdint>

class BinaryReadStream {
public:
	BinaryReadStream();
	virtual ~BinaryReadStream()=0;
	
	BinaryReadStream(const BinaryReadStream &)=delete;
	BinaryReadStream &operator=(const BinaryReadStream &)=delete;
	
	virtual void setPos(size_t newPos)=0;
	virtual size_t getPos() const noexcept =0;
	virtual size_t read(uint8_t *buff, size_t size)=0;
	virtual size_t getLength() const noexcept =0;
};

class BinaryWriteStream {
public:
	BinaryWriteStream();
	virtual ~BinaryWriteStream()=0;
	
	BinaryWriteStream(const BinaryWriteStream &)=delete;
	BinaryWriteStream &operator=(const BinaryWriteStream &)=delete;
	
	virtual void setPos(size_t newPos)=0;
	virtual size_t getPos() const noexcept =0;
	virtual size_t write(const uint8_t *buff, size_t size)=0;
	virtual size_t getLength() const noexcept =0;
};

class ManualPosBinaryReadStream : public BinaryReadStream {
public:
	void setPos(size_t newPos) override;
	size_t getPos() const noexcept override;
protected:
	ManualPosBinaryReadStream();
	size_t position;
};

class ManualPosBinaryWriteStream : public BinaryWriteStream {
public:
	void setPos(size_t newPos) override;
	size_t getPos() const noexcept override;
protected:
	ManualPosBinaryWriteStream();
	size_t position;
};

#endif