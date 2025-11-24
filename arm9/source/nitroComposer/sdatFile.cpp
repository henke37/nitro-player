#include "sdatFile.h"

#include "../fileStream.h"
#include "../substream.h"
#include "../binaryReader.h"

#include "sseq.h"
#include "strm.h"
#include "sbnk.h"
#include "swar.h"

#include <nds/arm9/sassert.h>

namespace NitroComposer {

	SDatFile::SDatFile(const std::string &fileName) {
		mainStream = std::make_unique<FileReadStream>(fileName);
		Load();
	}

	SDatFile::SDatFile(std::unique_ptr<BinaryReadStream> stream) {
		sassert(stream, "no sdat stream?");
		mainStream = std::move(stream);
		Load();
	}

	std::unique_ptr<SBNK> SDatFile::OpenBank(const std::unique_ptr<BankInfoRecord> &info) const {
		if(!info) return std::unique_ptr<SBNK>();

		return std::make_unique<SBNK>(this->OpenFile(info->fatId));
	}

	std::unique_ptr<STRM> SDatFile::OpenStream(const std::unique_ptr<StreamInfoRecord> &info) const {
		if(!info) return std::unique_ptr<STRM>();

		return std::make_unique<STRM>(this->OpenFile(info->fatId));
	}

	std::unique_ptr<SSEQ> SDatFile::OpenSequence(const std::unique_ptr<SequenceInfoRecord> &info) const {
		if(!info) return std::unique_ptr<SSEQ>();

		return std::make_unique<SSEQ>(this->OpenFile(info->fatId));
	}
	std::unique_ptr<SWAR> SDatFile::OpenWaveArchive(const std::unique_ptr<WaveArchiveInfoRecord> &info) const {
		if(!info) return std::unique_ptr<SWAR>();

		return std::make_unique<SWAR>(this->OpenFile(info->fatId));
	}
	std::unique_ptr<STRM> SDatFile::OpenStream(unsigned int streamId) const {
		auto &info = streamInfos[streamId];
		if(!info) return std::unique_ptr<STRM>();

		return std::make_unique<STRM>(this->OpenFile(info->fatId));
	}
	std::unique_ptr<SWAR> SDatFile::OpenWaveArchive(unsigned int archiveId) const {
		auto &info = waveArchInfos[archiveId];
		if(!info) return std::unique_ptr<SWAR>();

		return std::make_unique<SWAR>(this->OpenFile(info->fatId));
	}

	const std::unique_ptr<SequenceInfoRecord> &SDatFile::GetSequenceInfo(unsigned int sequenceId) const {
		return sequenceInfos[sequenceId];
	}

	const std::unique_ptr<SequenceInfoRecord> &SDatFile::GetSequenceInfo(const std::string &sequenceName) const {
		for(std::size_t sequenceId = 0; sequenceId < sequenceNames.size(); ++sequenceId) {
			if(sequenceName != sequenceNames[sequenceId]) continue;
			return sequenceInfos[sequenceId];
		}
		sassert(0, "Unknown sequence \"%s\"!", sequenceName.c_str());
	}

	const std::unique_ptr<BankInfoRecord> &SDatFile::GetBankInfo(unsigned int bankId) const {
		return bankInfos[bankId];
	}

	const std::unique_ptr<WaveArchiveInfoRecord> &SDatFile::GetWaveArchiveInfo(unsigned int archiveId) const {
		return waveArchInfos[archiveId];
	}

	const std::unique_ptr<PlayerInfoRecord> &SDatFile::GetPlayerInfo(unsigned int playerId) const {
		return playerInfos.at(playerId);
	}

	const std::unique_ptr<StreamInfoRecord> &SDatFile::GetStreamInfo(unsigned int streamId) const {
		return streamInfos.at(streamId);
	}

	const std::unique_ptr<StreamInfoRecord> &SDatFile::GetStreamInfo(const std::string &streamName) const {
		for(std::size_t streamId = 0; streamId < streamNames.size(); ++streamId) {
			if(streamName != streamNames[streamId]) continue;
			return streamInfos[streamId];
		}
		sassert(0, "Unknown stream \"%s\"!", streamName.c_str());
	}

	std::string SDatFile::GetNameForBank(unsigned int bankId) const {
		if(bankId >= bankNames.size()) {
			return std::string("BANK_") + std::to_string(bankId);
		}
		std::string name = bankNames.at(bankId);
		if(name.empty()) {
			return std::string("BANK_") + std::to_string(bankId);
		}
		return name;
	}

	std::string SDatFile::GetNameForWaveArchive(unsigned int archiveId) const {
		if(archiveId >= waveArchiveNames.size()) {
			return std::string("WAVE_") + std::to_string(archiveId);
		}
		std::string name = waveArchiveNames.at(archiveId);
		if(name.empty()) {
			return std::string("WAVE_") + std::to_string(archiveId);
		}
		return name;
	}

	std::string SDatFile::GetNameForSequence(unsigned int sequenceId) const {
		if(sequenceId >= sequenceNames.size()) {
			return std::string("SEQ_") + std::to_string(sequenceId);
		}
		std::string name = sequenceNames.at(sequenceId);
		if(name.empty()) {
			return std::string("SEQ_") + std::to_string(sequenceId);
		}
		return name;
	}

	std::string SDatFile::GetNameForStream(unsigned int streamId) const {
		if(streamId >= streamNames.size()) {
			return std::string("STRM_") + std::to_string(streamId);
		}
		std::string name = streamNames.at(streamId);
		if(name.empty()) {
			return std::string("STRM_") + std::to_string(streamId);
		}
		return name;
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
		auto &record = fat[fileId];
		return std::make_unique<SubStream>(mainStream.get(), record.offset, record.size, false);
	}

	void SDatFile::parseSymb(std::uint32_t offset, std::uint32_t size) {
		auto strm = std::make_unique<SubStream>(mainStream.get(), offset, size, false);
		BinaryReader reader(std::move(strm));

		std::string signature = reader.readString(4);
		sassert(signature == "SYMB", "bad SYMB signature");

		reader.skip(4);

		std::uint32_t seqSymbPtr = reader.readLELong();
		std::uint32_t seqArchSymbPtr = reader.readLELong();
		std::uint32_t bankSymbPtr = reader.readLELong();
		std::uint32_t waveArchSymbPtr = reader.readLELong();
		std::uint32_t playerSymbPtr = reader.readLELong();
		std::uint32_t groupSymbPtr = reader.readLELong();
		std::uint32_t player2SymbPtr = reader.readLELong();
		std::uint32_t streamSymbPtr = reader.readLELong();

		auto parseSymbSubRec = [&](std::uint32_t ptr) {
			std::vector<std::string> names;
			reader.setPos(ptr);

			std::uint32_t nameCount = reader.readLELong();
			names.reserve(nameCount);

			for(std::uint32_t nameIndex = 0; nameIndex < nameCount; ++nameIndex) {
				std::uint32_t strPos = reader.readLELong();

				if(strPos == 0) {
					names.emplace_back();
					continue;
				}

				std::uint32_t pos = reader.getPos();

				//printf("%lu %lu\n", pos, strPos);

				reader.setPos(strPos);
				names.emplace_back(reader.readZeroTermString());
				reader.setPos(pos);
			}
			return names;
			};

		sequenceNames = parseSymbSubRec(seqSymbPtr);
		bankNames = parseSymbSubRec(bankSymbPtr);
		waveArchiveNames = parseSymbSubRec(waveArchSymbPtr);
		playerNames = parseSymbSubRec(playerSymbPtr);
		groupNames = parseSymbSubRec(groupSymbPtr);
		streamPlayerNames = parseSymbSubRec(player2SymbPtr);
		streamNames = parseSymbSubRec(streamSymbPtr);

		{
			reader.setPos(seqArchSymbPtr);
			std::uint32_t archNameCount = reader.readLELong();
			sequenceArchiveNames.reserve(archNameCount);

			for(std::uint32_t archiveIndex = 0; archiveIndex < archNameCount; ++archiveIndex) {
				std::uint32_t archNamePos = reader.readLELong();
				std::uint32_t seqListPos = reader.readLELong();

				std::uint32_t nextPos = reader.getPos();

				SequenceArchiveNames archNames;

				if(archNamePos) {
					reader.setPos(archNamePos);
					archNames.archiveName = reader.readZeroTermString();
				}
				if(seqListPos) {
					archNames.sequenceNames = parseSymbSubRec(seqListPos);
				}
				sequenceArchiveNames.push_back(std::move(archNames));

				reader.setPos(nextPos);
			}
		}
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
			//printf("Loaded %u sequence infos\n", sequenceInfos.size());
		}

		{
			std::vector<std::uint32_t> recordPositions = readChunkPositions(2);
			bankInfos.reserve(recordPositions.size());

			for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
				std::uint32_t offset = *itr;
				if(offset == 0) {
					bankInfos.emplace_back(nullptr);
					continue;
				}
				reader.setPos(offset);

				std::unique_ptr<BankInfoRecord> record = std::make_unique<BankInfoRecord>();
				record->fatId = reader.readLEShort();
				reader.skip(2);
				for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
					record->swars[swarIndex] = reader.readLEShort();
				}
				bankInfos.emplace_back(std::move(record));
			}
			//printf("Loaded %u bank infos\n", bankInfos.size());
		}

		{
			std::vector<std::uint32_t> recordPositions = readChunkPositions(3);
			waveArchInfos.reserve(recordPositions.size());

			for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
				std::uint32_t offset = *itr;
				if(offset == 0) {
					waveArchInfos.emplace_back(nullptr);
					continue;
				}
				reader.setPos(offset);

				std::unique_ptr<WaveArchiveInfoRecord> record = std::make_unique<WaveArchiveInfoRecord>();
				record->fatId = reader.readLEShort();
				waveArchInfos.emplace_back(std::move(record));
			}
			//printf("Loaded %u wave arch infos\n", waveArchInfos.size());
		}

		{
			std::vector<std::uint32_t> recordPositions = readChunkPositions(4);
			playerInfos.reserve(recordPositions.size());

			for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
				std::uint32_t offset = *itr;
				if(offset == 0) {
					playerInfos.emplace_back(nullptr);
					continue;
				}
				reader.setPos(offset);

				std::unique_ptr<PlayerInfoRecord> record = std::make_unique<PlayerInfoRecord>();
				record->maxSequences = reader.readByte();
				record->channelMask = reader.readLEShort();
				record->heapSize = reader.readLELong();
				playerInfos.emplace_back(std::move(record));
			}
			//printf("Loaded %u player infos\n", playerInfos.size());
		}

		{
			std::vector<std::uint32_t> recordPositions = readChunkPositions(7);
			streamInfos.reserve(recordPositions.size());

			for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
				std::uint32_t offset = *itr;
				if(offset == 0) {
					streamInfos.emplace_back(nullptr);
					continue;
				}
				reader.setPos(offset);

				std::unique_ptr<StreamInfoRecord> record = std::make_unique<StreamInfoRecord>();
				record->fatId = reader.readLEShort();
				record->vol = reader.readByte();
				record->priority = reader.readByte();
				record->player = reader.readByte();
				record->forceStereo = reader.readByte() != 0;
				streamInfos.emplace_back(std::move(record));
			}
			//printf("Loaded %u stream infos\n", streamInfos.size());
		}

		{
			std::vector<std::uint32_t> recordPositions = readChunkPositions(3);
			sequenceArchInfos.reserve(recordPositions.size());

			for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
				std::uint32_t offset = *itr;
				if(offset == 0) {
					sequenceArchInfos.emplace_back(nullptr);
					continue;
				}
				reader.setPos(offset);

				std::unique_ptr<SequenceArchiveRecord> record = std::make_unique<SequenceArchiveRecord>();
				record->fatId = reader.readLEShort();
				sequenceArchInfos.emplace_back(std::move(record));
			}
			//printf("Loaded %u sequence arch infos\n", sequenceArchInfos.size());
		}

		{
			std::vector<std::uint32_t> recordPositions = readChunkPositions(5);
			groupInfos.reserve(recordPositions.size());

			for(auto itr = recordPositions.begin(); itr != recordPositions.end(); ++itr) {
				std::uint32_t offset = *itr;
				if(offset == 0) {
					groupInfos.emplace_back(nullptr);
					continue;
				}
				reader.setPos(offset);

				std::unique_ptr<GroupInfoRecord> record = std::make_unique<GroupInfoRecord>();
				std::uint32_t elementCount = reader.readLELong();
				record->elements.reserve(elementCount);
				for(std::uint32_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
					std::uint16_t type = reader.readLEShort();
					std::uint16_t id = reader.readLEShort();
					record->elements.emplace_back(static_cast<GroupInfoRecord::ElementType>(type), id);
				}
				groupInfos.emplace_back(std::move(record));
			}
			//printf("Loaded %u group infos\n", groupInfos.size());
		}
	}

	void SDatFile::parseFat(std::uint32_t offset, std::uint32_t size) {
		auto strm = std::make_unique<SubStream>(mainStream.get(), offset, size, false);
		BinaryReader reader(std::move(strm));

		std::string signature = reader.readString(4);
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

}