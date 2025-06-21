#include "sdatFile.h"

#include "../fileStream.h"
#include "../substream.h"
#include "../binaryReader.h"

#include <nds/arm9/sassert.h>

SDatFile::SDatFile(const std::string &fileName) {
	mainStream = std::make_unique<FileReadStream>(fileName);
	Load();
}

SDatFile::SDatFile(std::unique_ptr<BinaryReadStream> stream) {
	sassert(stream, "no sdat stream?");
	mainStream = std::move(stream);
	Load();
}

void SDatFile::Load() {
	BinaryReader reader(mainStream.get(), false);

	std::string signature = reader.readString(4);
	sassert(signature == "SDAT", "Bad signature, not a SDAT archive!");
	std::uint16_t bom = reader.readLEShort();
	sassert(bom == 0xFEFF, "Bad BOM");
	std::uint16_t version = reader.readLEShort();
	sassert(version == 0x0100, "Bad version");
	reader.skip(4);//file size
	reader.skip(2);//header size
	reader.skip(2);//chunk count

	std::uint32_t symbOffset = reader.readLELong();
	std::uint32_t symbSize = reader.readLELong();

	std::uint32_t infoOffset = reader.readLELong();
	std::uint32_t infoSize = reader.readLELong();

	std::uint32_t fatOffset = reader.readLELong();
	std::uint32_t fatSize = reader.readLELong();

	//don't need 'em
	//std::uint32_t fileOffset = reader.readLELong();
	//std::uint32_t fileSize = reader.readLELong();

	if(symbOffset) {
		parseSymb(symbOffset, symbSize);
	}

	parseInfo(infoOffset, infoSize);
	parseFat(fatOffset, fatSize);
}

std::unique_ptr<BinaryReadStream> SDatFile::OpenFile(unsigned int fileId) const {
	sassert(fileId < fat.size(), "File %u past end of FAT", fileId);
	return std::unique_ptr<BinaryReadStream>();
}

void SDatFile::parseSymb(std::uint32_t offset, std::uint32_t size) {
	auto strm = std::make_unique<SubStream>(mainStream.get(), offset, size, false);
	BinaryReader reader(std::move(strm));

	std::string signature = reader.readString(4);
	sassert(signature == "SYMB", "bad SYMB signature");
}

void SDatFile::parseInfo(std::uint32_t offset, std::uint32_t size) {
	auto strm = std::make_unique<SubStream>(mainStream.get(), offset, size, false);
	BinaryReader reader(std::move(strm));

	std::string signature = reader.readString(4);
	sassert(signature == "INFO", "bad INFO signature");

	reader.skip(4);//size

	std::uint32_t subChunkPositions[8];
	for(unsigned int i = 0; i < 8; ++i) {
		subChunkPositions[i] = reader.readLELong();
	}

	auto readChunkPositions = [&](unsigned int chunkId) {
		reader.setPos(subChunkPositions[chunkId]);
		std::uint32_t recordCount = reader.readLELong();
		std::vector<std::uint32_t> recordPositions;
		recordPositions.reserve(recordCount);
		for(unsigned int recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
			recordPositions.emplace_back(reader.readLELong());
		}
		return recordPositions;
	};

	{
		std::vector<std::uint32_t> recordPositions = readChunkPositions(0);
		sequenceInfos.reserve(recordPositions.size());
		return;
		for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
			std::uint32_t offset = *itr;
			if(offset == 0) {
				sequenceInfos.emplace_back(nullptr);
				continue;
			}
			reader.setPos(offset);

			std::unique_ptr<SequenceInfoRecord> record = std::make_unique<SequenceInfoRecord>();
			record->fatId = reader.readLEShort();
			reader.skip(2);
			record->bankId = reader.readLEShort();
			record->vol = reader.readByte();
			record->channelPriority = reader.readByte();
			record->playerPriority = reader.readByte();
			record->player = reader.readByte();
			sequenceInfos.emplace_back(std::move(record));
		}
		printf("Loaded %u seqInfos", sequenceInfos.size());
	}
}

void SDatFile::parseFat(std::uint32_t offset, std::uint32_t size) {
	auto strm = std::make_unique<SubStream>(mainStream.get(), offset, size, false);
	BinaryReader reader(std::move(strm));

	std::string signature=reader.readString(4);
	sassert(signature == "FAT ", "bad FAT signature");

	reader.skip(4);//size

	std::uint32_t fileCount = reader.readLELong();

	for(std::uint32_t fileIndex = 0; fileIndex < fileCount; ++fileIndex) {
		std::uint32_t offset = reader.readLELong();
		std::uint32_t size = reader.readLELong();
		reader.skip(8);
		fat.emplace_back(offset, size);
	}
}
