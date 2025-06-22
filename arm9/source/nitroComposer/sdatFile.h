#ifndef NITROCOMPOSER_SDATFILE_H
#define NITROCOMPOSER_SDATFILE_H

#include <memory>
#include <string>
#include <vector>

#include "../binaryStream.h"

#include "infoRecords.h"

class SBNK;
class STRM;
class SSEQ;
class SWAR;

class SDatFile {
public:
	SDatFile(const std::string &fileName);
	SDatFile(std::unique_ptr<BinaryReadStream> stream);

	std::unique_ptr<SBNK> OpenBank(unsigned int bankId);
	std::unique_ptr<STRM> OpenStream(unsigned int streamId);
	std::unique_ptr<SSEQ> OpenSequence(unsigned int sequenceId);
	std::unique_ptr<SWAR> OpenWaveArchive(unsigned int archiveId);

	std::unique_ptr<SBNK> OpenBank(std::string bankName);
	std::unique_ptr<STRM> OpenStream(std::string streamName);
	std::unique_ptr<SSEQ> OpenSequence(std::string sequenceName);
	std::unique_ptr<SWAR> OpenWaveArchive(std::string archiveName);

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
	std::vector<std::unique_ptr<BankInfoRecord>> bankInfos;
	std::vector<std::unique_ptr<WaveArchiveInfoRecord>> waveArchInfos;
	std::vector<std::unique_ptr<SequenceArchiveRecord>> sequenceArchInfos;
	std::vector<std::unique_ptr<PlayerInfoRecord>> playerInfos;
	std::vector<std::unique_ptr<StreamInfoRecord>> streamInfos;
	std::vector<std::unique_ptr<GroupInfoRecord>> groupInfos;

	struct SequenceArchiveNames {
		std::string archiveName;
		std::vector<std::string> sequenceNames;
	};

	std::vector<std::string> sequenceNames;
	std::vector<SequenceArchiveNames> sequenceArchiveNames;
	std::vector<std::string> bankNames;
	std::vector<std::string> waveArchiveNames;
	std::vector<std::string> playerNames;
	std::vector<std::string> groupNames;
	std::vector<std::string> streamPlayerNames;
	std::vector<std::string> streamNames;

	void parseSymb(std::uint32_t offset, std::uint32_t size);
	void parseInfo(std::uint32_t offset, std::uint32_t size);
	void parseFat(std::uint32_t offset, std::uint32_t size);
};

#endif