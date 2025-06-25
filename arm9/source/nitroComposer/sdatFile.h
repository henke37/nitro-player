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

	std::unique_ptr<STRM> OpenStream(unsigned int streamId) const;
	std::unique_ptr<SWAR> OpenWaveArchive(unsigned int archiveId) const;

	std::unique_ptr<SBNK> OpenBank(const std::unique_ptr<BankInfoRecord>&) const;
	std::unique_ptr<STRM> OpenStream(const std::unique_ptr<StreamInfoRecord>&) const;
	std::unique_ptr<SSEQ> OpenSequence(const std::unique_ptr<SequenceInfoRecord> &) const;
	std::unique_ptr<SWAR> OpenWaveArchive(const std::unique_ptr<WaveArchiveInfoRecord> &) const;

	const std::unique_ptr<SequenceInfoRecord> &GetSequenceInfo(unsigned int sequenceId) const;
	const std::unique_ptr<SequenceInfoRecord> &GetSequenceInfo(const std::string &sequenceName) const;
	const std::unique_ptr<BankInfoRecord> &GetBankInfo(unsigned int bankId) const;
	const std::unique_ptr<BankInfoRecord> &GetBankInfo(const std::string &bankName) const;
	const std::unique_ptr<WaveArchiveInfoRecord> &GetWaveArchiveInfo(unsigned int archiveId) const;
	const std::unique_ptr<WaveArchiveInfoRecord> &GetWaveArchiveInfo(const std::string &archiveName) const;

	std::string GetNameForSequence(unsigned int sequenceId) const;
	std::string GetNameForBank(unsigned int bankId) const;
	std::string GetNameForWaveArchive(unsigned int archiveId) const;

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