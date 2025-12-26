#include "fileStream.h"

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <nds/arm9/sassert.h>

FileReadStream::FileReadStream(const std::string &fileName) {
	fileHandle=::open(fileName.c_str(), O_RDONLY);
	
	sassert(fileHandle>=0, "Failed to open \"%s\"!", fileName.c_str());
}

FileReadStream::~FileReadStream() {
}

FileReadStream::FileReadStream(int fileHandle) : BaseFileStream(fileHandle) {}

std::unique_ptr<FileReadStream> FileReadStream::tryOpen(const std::string &fileName) {
	int fileHandle = ::open(fileName.c_str(), O_RDONLY);

	if(fileHandle<0) return std::unique_ptr<FileReadStream>();

	return std::unique_ptr<FileReadStream>(new FileReadStream(fileHandle));
}

void BaseFileStream::setPos(size_t newPos) {
	::lseek(fileHandle, newPos, SEEK_SET);
}

size_t BaseFileStream::getPos() const {
	return ::lseek(fileHandle, 0, SEEK_CUR);
}

size_t FileReadStream::read(uint8_t *buf, size_t readSize) {
	auto readCnt = ::read(fileHandle, buf, readSize);
	sassert(readCnt >= 0, "Read returned %zi!", readCnt);
	return readCnt;
}

size_t BaseFileStream::getLength() const {
	struct stat s;
	fstat(fileHandle, &s);
	return s.st_size;
}

BaseFileStream::BaseFileStream() {}

BaseFileStream::BaseFileStream(int fileHandle) : fileHandle(fileHandle) {}

BaseFileStream::~BaseFileStream() {
	::close(fileHandle);
}

FileWriteStream::FileWriteStream(const std::string &fileName, bool allowExisting) {
	fileHandle = ::open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC | (allowExisting ? 0 :O_EXCL));

	sassert(fileHandle >= 0, "Failed to open \"%s\"!", fileName.c_str());
}
FileWriteStream::FileWriteStream(int fileHandle) : BaseFileStream(fileHandle) {}

FileWriteStream::~FileWriteStream() {}

std::unique_ptr<FileWriteStream> FileWriteStream::tryOpen(const std::string &fileName) {
	int fileHandle = ::open(fileName.c_str(), O_WRONLY | O_TRUNC);

	if(fileHandle < 0) return std::unique_ptr<FileWriteStream>();

	return std::unique_ptr<FileWriteStream>(new FileWriteStream(fileHandle));
}

size_t FileWriteStream::write(const uint8_t *buff, size_t size) {
	ssize_t result = ::write(fileHandle, buff, size);
	sassert(result >= 0, "Write returned %zi!", result);
	return result;
}