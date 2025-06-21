#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include "binaryStream.h"

#include <string>
#include <memory>

class BaseFileStream {
public:
	void setPos(size_t newPos);
	size_t getPos() const;
	size_t getLength() const;
protected:
	BaseFileStream();
	BaseFileStream(int fileHandle);
	~BaseFileStream();
	int fileHandle;
};

class FileReadStream : public BinaryReadStream, protected BaseFileStream {
public:
	FileReadStream(const std::string &fileName);
	~FileReadStream();

	static std::unique_ptr<FileReadStream> tryOpen(const std::string &fileName);
	
	void setPos(size_t newPos) override { BaseFileStream::setPos(newPos); }
	size_t getPos() const override { return BaseFileStream::getPos(); }
	size_t getLength() const override { return BaseFileStream::getLength(); }

	size_t read(uint8_t *buf, size_t size) override;
	
private:
	FileReadStream(int fileHandle);
};

class FileWriteStream : public BinaryWriteStream, protected BaseFileStream {
public:
	FileWriteStream(const std::string &fileName, bool allowExisting);
	~FileWriteStream();

	static std::unique_ptr<FileWriteStream> tryOpen(const std::string &fileName);

	void setPos(size_t newPos) override { BaseFileStream::setPos(newPos); }
	size_t getPos() const override { return BaseFileStream::getPos(); }
	size_t getLength() const override { return BaseFileStream::getLength(); }

	size_t write(const uint8_t *buff, size_t size) override;

private:
	FileWriteStream(int fileHandle);
};

#endif