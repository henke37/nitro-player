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

NDSFile::FileSystem::Iterator NDSFile::getFileSystemIterator() const {
	return fileSystem->getIterator();
}

NDSFile::Banner NDSFile::GetBanner() const {
	return Banner(std::make_unique<SubStream>(stream.get(), bannerOffset, 0x23C0, false));
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

	reader.skip(4 * 4 + 4 * 2); //overlays and slot1 settings

	bannerOffset = reader.readLELong();

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

const NDSFile::FileSystem::Directory *NDSFile::FileSystem::getDir(std::uint16_t dirId) const {
	assert(dirId >= Directory::DirEntry::folderThreshold);
	return &directories.at(dirId - Directory::DirEntry::folderThreshold);
}

const NDSFile::FileSystem::Directory *NDSFile::FileSystem::getRootDir() const {
	return &*directories.cbegin();
}

std::uint16_t NDSFile::FileSystem::ResolvePath(const std::string &path) const {
	size_t curSlashPos = 0;
	size_t partStartPos = 0;
	const Directory *currDir = &directories.at(0);
	for(;;) {
		curSlashPos = path.find('/', partStartPos + 1);

		if(curSlashPos == std::string::npos) {
			std::string file = path.substr(partStartPos);

			auto entry = currDir->findEntry(file);
			if(!entry) return Directory::DirEntry::invalidFileId;
			assert(entry->fileId < Directory::DirEntry::folderThreshold);
			return entry->fileId;
		}

		std::string folder = path.substr(partStartPos, curSlashPos - partStartPos);

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

NDSFile::FileSystem::Iterator NDSFile::FileSystem::getIterator() const {
	return NDSFile::FileSystem::Iterator(this);
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

NDSFile::FileSystem::Iterator::Iterator(const FileSystem *fileSystem) : fileSystem(fileSystem) {
	dir = fileSystem->getRootDir();
	dirItr = dir->entries.cbegin();
}

NDSFile::FileSystem::Iterator::Iterator(std::nullptr_t) : fileSystem(nullptr), dir(nullptr) {}

const NDSFile::FileSystem::Directory::DirEntry *NDSFile::FileSystem::Iterator::current() const {
	assert(fileSystem);
	assert(dir);
	assert(dirItr != dir->entries.cend());

	return &*dirItr;
}

bool NDSFile::FileSystem::Iterator::operator==(const Iterator &other) const {
	if(this->fileSystem!=other.fileSystem) return false;
	if(this->dir != other.dir) return false;
	return this->dirItr == other.dirItr;
}
bool NDSFile::FileSystem::Iterator::operator!=(const Iterator &other) const {
	if(this->fileSystem != other.fileSystem) return true;
	if(this->dir != other.dir) return true;
	return this->dirItr != other.dirItr;
}

bool NDSFile::FileSystem::Iterator::atEnd() const {
	if(!fileSystem) return true;
	if(!dir) return true;
	if(!dir->isRoot()) return false;
	return dirItr == dir->entries.cend();
}

std::string NDSFile::FileSystem::Iterator::getFullPath() const {
	std::string path = getFileName();

	auto curDir = dir;

	while(!curDir->isRoot()) {
		auto parentDir = fileSystem->getDir(curDir->parentId);
		auto entryInParent = entryInParentDir(curDir);

		path = entryInParent->name + "/" + path;
		curDir = parentDir;
	}

	return path;
}

void NDSFile::FileSystem::Iterator::operator++() {
	assert(fileSystem);
	assert(dir);

	if(dirItr->isDirectory()) {
		dir = fileSystem->getDir(dirItr->fileId);
		dirItr = dir->entries.cbegin();
		return;
	}

	++dirItr;

	while(dirItr == dir->entries.cend()) {
		if(dir->isRoot()) {
			dir = nullptr;
			return;
		}
		goUp();
	}
}

void NDSFile::FileSystem::Iterator::goUp() {
	assert(fileSystem);
	assert(dir);
	sassert(!dir->isRoot(), "Can't go up from the root!");

	auto parentDir = fileSystem->getDir(dir->parentId);
	for(auto parentItr = parentDir->entries.cbegin(); parentItr != parentDir->entries.cend(); ++parentItr) {
		auto &entryInParentDir = *parentItr;
		if(!entryInParentDir.isDirectory()) continue;
		auto candidateDir = fileSystem->getDir(entryInParentDir.fileId);
		if(candidateDir != dir) continue;

		dir = parentDir;
		dirItr = parentItr + 1;

		return;
	}
	sassert(0, "Failed to find dirEntry in parent!");
}

const NDSFile::FileSystem::Directory::DirEntry *NDSFile::FileSystem::Iterator::entryInParentDir(const Directory *dir) const {
	assert(fileSystem);
	assert(dir);
	sassert(!dir->isRoot(), "Root directory has no parent!");
	auto parentDir = fileSystem->getDir(dir->parentId);
	for(auto parentItr = parentDir->entries.cbegin(); parentItr != parentDir->entries.cend(); ++parentItr) {
		auto &entryInParentDir = *parentItr;
		if(!entryInParentDir.isDirectory()) continue;
		auto candidateDir = fileSystem->getDir(entryInParentDir.fileId);
		if(candidateDir != dir) continue;
		return &*parentItr;
	}
	sassert(0, "Failed to find dirEntry in parent!");
	return nullptr;
}

NDSFile::Banner::Banner(std::unique_ptr<BinaryReadStream> &&stream) {
	BinaryReader reader(std::move(stream));
	version = static_cast<Version>(reader.readLEShort());
	reader.skip(0x0240 - 2); //skip to titles
	unsigned int existingTitleCount = version == Version::Original ? 6 : 8;

	for(unsigned int i = 0; i < existingTitleCount; ++i) {
		titles[i] = reader.readLEUTF16String(128);

		//trim trailing nulls
		size_t nullPos = titles[i].find(u'\0');
		if(nullPos != std::u16string::npos) {
			titles[i] = titles[i].substr(0, nullPos);
		}
	}
}
