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
		};
		std::vector<Directory> directories;

		BinaryReadStream *fileData;

		std::uint16_t ResolvePath(const std::string &name) const;

		void ParseFAT(std::unique_ptr<BinaryReadStream> &&FATData);
		void ParseFNT(std::unique_ptr<BinaryReadStream> &&FNTData);
	};

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