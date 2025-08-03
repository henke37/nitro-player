#include "ndsfile.h"

#include "binaryReader.h"
#include "substream.h"
#include "fileStream.h"

#include <cassert>
#include <nds/arm9/sassert.h>

NDSFile::NDSFile(std::unique_ptr<BinaryReadStream> &&stream) : stream(std::move(stream)) {
	assert(this->stream);
	Parse();
}

NDSFile::NDSFile(const std::string &fileName) {
	stream = std::make_unique<FileReadStream>(fileName);
	Parse();
}

NDSFile::~NDSFile() {}

std::unique_ptr<BinaryReadStream> NDSFile::OpenFile(std::uint16_t id) const {
	return fileSystem->OpenFile(id);
}

std::unique_ptr<BinaryReadStream> NDSFile::OpenFile(const std::string &path) const {
	return fileSystem->OpenFile(path);
}

void NDSFile::Parse() {
	BinaryReader reader(stream.get(), false);

	reader.skip(12);//title
	gameCode = reader.readString(4);
	makerCode = reader.readLEShort();
	unitCode = reader.readByte();
	reader.skip(1 + 1 + 7 + 1);
	region = reader.readByte();
	version = reader.readByte();
	autoStart = reader.readByte() != 0;

	arm9.Offset = reader.readLELong();
	arm9.EntryPoint = reader.readLELong();
	arm9.RamAddress = reader.readLELong();
	arm9.Size = reader.readLELong();

	arm7.Offset = reader.readLELong();
	arm7.EntryPoint = reader.readLELong();
	arm7.RamAddress = reader.readLELong();
	arm7.Size = reader.readLELong();

	std::uint32_t FNTOffset = reader.readLELong();
	std::uint32_t FNTSize = reader.readLELong();
	std::uint32_t FATOffset = reader.readLELong();
	std::uint32_t FATSize = reader.readLELong();

	std::unique_ptr<BinaryReadStream> FNTData = std::make_unique<SubStream>
		(stream.get(), FNTOffset, FNTSize, false);
	std::unique_ptr<BinaryReadStream> FATData = std::make_unique<SubStream>
		(stream.get(), FATOffset, FATSize, false);

	fileSystem = std::make_unique<FileSystem>(std::move(FNTData), std::move(FATData), stream.get());
}

NDSFile::FileSystem::FileSystem(std::unique_ptr<BinaryReadStream> &&FNTData,
	std::unique_ptr<BinaryReadStream> &&FATData,
	BinaryReadStream *fileData) : fileData(fileData) {
	assert(fileData);
	assert(FATData);
	assert(FNTData);
	ParseFAT(std::move(FATData));
	ParseFNT(std::move(FNTData));
}

const NDSFile::FileSystem::Directory::DirEntry *NDSFile::FileSystem::Directory::findEntry(const std::string &name) const {
	for(auto dirItr = entries.cbegin(); dirItr != entries.cend(); ++dirItr) {
		auto &dirEntry = *dirItr;
		if(dirEntry.name != name) continue;
		return &dirEntry;
	}
	return nullptr;
}

std::uint16_t NDSFile::FileSystem::ResolvePath(const std::string &path) const {
	size_t curSlashPos = 0;
	size_t partStartPos = 0;
	const Directory *currDir = &directories.at(0);
	for(;;) {
		curSlashPos = path.find('/', partStartPos + 1);

		if(curSlashPos == std::string::npos) {
			std::string file = path.substr(partStartPos);

			printf("File \"%s\"\n", file.c_str());

			auto entry = currDir->findEntry(file);
			if(!entry) return Directory::DirEntry::invalidFileId;
			assert(entry->fileId < Directory::DirEntry::folderThreshold);
			return entry->fileId;
		}

		std::string folder = path.substr(partStartPos, curSlashPos - partStartPos);

		printf("Path \"%s\"\n",folder.c_str());

		auto entry = currDir->findEntry(folder);
		if(!entry) return Directory::DirEntry::invalidFileId;
		assert(entry->fileId >= Directory::DirEntry::folderThreshold);
		std::uint16_t dirId = entry->fileId - Directory::DirEntry::folderThreshold;
		currDir = &directories.at(dirId);

		partStartPos = curSlashPos + 1;
	}
	return Directory::DirEntry::invalidFileId;
}

void NDSFile::FileSystem::ParseFAT(std::unique_ptr<BinaryReadStream> &&FATData) {
	std::uint16_t fileCount = FATData->getLength() / sizeof(FATRecord);
	fat.reserve(fileCount);

	BinaryReader reader(std::move(FATData));

	for(std::uint16_t fileIndex = 0; fileIndex < fileCount; ++fileIndex) {
		std::uint32_t start = reader.readLELong();
		std::uint32_t end = reader.readLELong();
		fat.emplace_back(start, end - start);
	}
}

void NDSFile::FileSystem::ParseFNT(std::unique_ptr<BinaryReadStream> &&FNTData) {
	BinaryReader reader(std::move(FNTData));

	std::vector<DirIndexEntry> dirIndexes;

	std::uint16_t numDirs;
	{
		std::uint32_t rootTableOffset = reader.readLELong();
		std::uint16_t firstFileId = reader.readLEShort();
		numDirs = reader.readLEShort();

		dirIndexes.reserve(numDirs);
		directories.reserve(numDirs);
		dirIndexes.emplace_back(rootTableOffset, firstFileId, Directory::DirEntry::invalidFileId);
	}

	for(std::uint16_t dirIndex = 1; dirIndex < numDirs; ++dirIndex) {
		std::uint32_t rootTableOffset = reader.readLELong();
		std::uint16_t firstFileId = reader.readLEShort();
		std::uint16_t parentDir = reader.readLEShort();
		dirIndexes.emplace_back(rootTableOffset, firstFileId, parentDir);
	}


	for(auto dirIndexItr = dirIndexes.cbegin(); dirIndexItr != dirIndexes.cend(); ++dirIndexItr) {
		auto &dirIndex = *dirIndexItr;
		reader.setPos(dirIndex.offset);
		std::uint16_t fileIndex = dirIndex.firstFileId;

		Directory dir;
		dir.parentId = dirIndex.parentId;

		for(;;) {
			std::uint8_t nameLen = reader.readByte();

			if(nameLen == 0) break;
			bool isDir = nameLen & 0x80;
			nameLen &= ~0x80;
			std::string name = reader.readString(nameLen);

			if(isDir) {
				dir.entries.emplace_back(name, reader.readLEShort());
			} else {
				dir.entries.emplace_back(name, fileIndex++);
			}
		}

		directories.push_back(dir);
	}
}

std::unique_ptr<BinaryReadStream> NDSFile::FileSystem::OpenFile(std::uint16_t id) const {
	auto &fatEntry = fat.at(id);
	return std::make_unique<SubStream>(this->fileData, fatEntry.offset, fatEntry.length, false);
}
std::unique_ptr<BinaryReadStream> NDSFile::FileSystem::OpenFile(const std::string &path) const {
	std::uint16_t id = ResolvePath(path);
	sassert(id != Directory::DirEntry::invalidFileId, "Couldn't resolve path \"%s\"!", path.c_str());
	return OpenFile(id);
}
