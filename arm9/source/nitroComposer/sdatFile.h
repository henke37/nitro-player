#ifndef NITROCOMPOSER_SDATFILE_H
#define NITROCOMPOSER_SDATFILE_H

#include <memory>
#include <string>
#include <vector>

#include "../binaryStream.h"

#include "infoRecords.h"

class SDatFile {
public:
	SDatFile(const std::string &fileName);
	SDatFile(std::unique_ptr<BinaryReadStream> stream);

private:

	void Load();

	std::unique_ptr<BinaryReadStream> OpenFile(unsigned int fileId) const;

	std::unique_ptr<BinaryReadStream> mainStream;

	struct FatRecord {
		std::uint32_t offset;
		std::uint32_t size;
		FatRecord(std::uint32_t offset, std::uint32_t size) : offset(offset), size(size) {}
	};

	std::vector<FatRecord> fat;


	std::vector<std::unique_ptr<SequenceInfoRecord>> sequenceInfos;

	void parseSymb(std::uint32_t offset, std::uint32_t size);
	void parseInfo(std::uint32_t offset, std::uint32_t size);
	void parseFat(std::uint32_t offset, std::uint32_t size);
};

#endif