#ifndef NDSFILE_H
#define NDSFILE_H

#include "binaryStream.h"

#include <memory>
#include <string>
#include <vector>

class NDSFile {
public:
	NDSFile(std::unique_ptr<BinaryReadStream> &&stream);
	NDSFile(const std::string &fileName);
	~NDSFile();

	std::unique_ptr<BinaryReadStream> OpenFile(std::uint16_t id) const;
	std::unique_ptr<BinaryReadStream> OpenFile(const std::string &path) const;

	const std::string &getGameCode() const { return gameCode; }
	std::uint16_t getMakerCode() const { return makerCode; }
	std::uint8_t getUnitCode() const { return unitCode; }
	std::uint8_t getRegion() const { return region; }

	class FileSystem {
	public:
		FileSystem(
			std::unique_ptr<BinaryReadStream> &&FNTData,
			std::unique_ptr<BinaryReadStream> &&FATData,
			BinaryReadStream *fileData);

		std::unique_ptr<BinaryReadStream> OpenFile(std::uint16_t id) const;
		std::unique_ptr<BinaryReadStream> OpenFile(const std::string &path) const;
	private:
		struct FATRecord {
			FATRecord(std::uint32_t offset, std::uint32_t length) : offset(offset), length(length) {}
			FATRecord(const FATRecord &) = default;
			~FATRecord() = default;
			FATRecord &operator=(const FATRecord &) = default;

			std::uint32_t offset;
			std::uint32_t length;
		};
		std::vector<FATRecord> fat;

		struct DirIndexEntry {
			DirIndexEntry(std::uint32_t offset,
				std::uint16_t firstFileId,
				std::uint16_t parentId) : 
				offset(offset), firstFileId(firstFileId), parentId(parentId) {}

			DirIndexEntry(const DirIndexEntry &) = default;
			~DirIndexEntry() = default;
			DirIndexEntry &operator=(const DirIndexEntry &) = default;

			std::uint32_t offset;
			std::uint16_t firstFileId;
			std::uint16_t parentId;
		};

		struct Directory {
			struct DirEntry {
				DirEntry(const std::string &name,
					std::uint16_t fileId) : name(name), fileId(fileId) {}

				DirEntry(const DirEntry &) = default;
				~DirEntry() = default;
				DirEntry &operator=(const DirEntry &) = default;

				bool isDirectory() const { return fileId >= folderThreshold; }

				std::string name;
				std::uint16_t fileId;

				static constexpr std::uint16_t folderThreshold = 0xF000;
				static constexpr std::uint16_t invalidFileId = 0xFFFF;
			};
			std::vector<DirEntry> entries;
			std::uint16_t parentId;
			const DirEntry *findEntry(const std::string &name) const;
			bool isRoot() const { return parentId == DirEntry::invalidFileId; }
		};
		std::vector<Directory> directories;

		const Directory *getDir(std::uint16_t dirId) const;
		const Directory *getRootDir() const;

		BinaryReadStream *fileData;

		std::uint16_t ResolvePath(const std::string &name) const;

		void ParseFAT(std::unique_ptr<BinaryReadStream> &&FATData);
		void ParseFNT(std::unique_ptr<BinaryReadStream> &&FNTData);
	public:
		class Iterator {
		public:
			explicit Iterator(std::nullptr_t);
			const Directory::DirEntry *current() const;
			const Directory::DirEntry *operator->() const { return current(); }
			const Directory::DirEntry &operator*() const { return *current(); }
			void operator++();
			bool operator==(const Iterator &other) const;
			bool operator!=(const Iterator &other) const;
			bool atEnd() const;

		private:
			Iterator(const FileSystem *fileSystem);

			const FileSystem *fileSystem;
			const Directory *dir;
			std::vector<Directory::DirEntry>::const_iterator dirItr;


			const Directory::DirEntry *entryInParentDir() const;
			void goUp();

			friend class ::NDSFile::FileSystem;
		};

		Iterator getIterator() const;
	};

	FileSystem::Iterator getFileSystemIterator() const;

private:	
	std::unique_ptr<BinaryReadStream> stream;

	std::string gameCode;
	std::uint16_t makerCode;
	std::uint8_t unitCode;
	std::uint8_t region;
	std::uint8_t version;
	bool autoStart;

	struct ExecutableData {
		std::uint32_t Offset;
		std::uint32_t EntryPoint;
		std::uint32_t RamAddress;
		std::uint32_t Size;
	};
	ExecutableData arm9, arm7;

	std::unique_ptr<FileSystem> fileSystem;

	void Parse();
};

#endif