#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/system.h>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

	SequencePlayer::SequencePlayer() : sdat(nullptr) {
		ipcPowerOn();

		powerOn(PM_SOUND_AMP);
	}

	void SequencePlayer::ipcPowerOn() {
		std::unique_ptr<BaseIPC> buff = std::make_unique<BaseIPC>();
		buff->command = BaseIPC::CommandType::PowerOn;
		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(BaseIPC), (u8 *)buff.get());
	}

	SequencePlayer::~SequencePlayer() {}

	void SequencePlayer::SetSdat(const SDatFile *sdat) {
		if(this->sdat == sdat) return;

		sseq.reset();
		sbnk.reset();
		for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
			loadedWaveArchives[swarIndex].Reset();
		}

		this->sdat = sdat;
	}

	void SequencePlayer::LoadSequence(unsigned int sequenceId) {
		const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceId);
		sassert(sequenceInfo, "Unknown sequence %u", sequenceId);
		LoadSequence(sequenceInfo);
	}

	void SequencePlayer::LoadSequence(const std::string &sequenceName) {
		const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceName);
		sassert(sequenceInfo, "Unknown sequence \"%s\"", sequenceName.c_str());
		LoadSequence(sequenceInfo);
	}

	void SequencePlayer::LoadSequence(const std::unique_ptr<SequenceInfoRecord> &sequenceInfo) {
		LoadBank(sequenceInfo->bankId);
		sseq = sdat->OpenSequence(sequenceInfo);

		auto stream = sseq->GetCommandStream();
		size_t dataLen = stream->getLength();

		sequenceData = std::make_unique<std::uint8_t[]>(dataLen);
		auto readLen = stream->read(sequenceData.get(), dataLen);

		sassert(readLen == dataLen, "failed reading sequence data");

		std::unique_ptr<PlayTrackIPC> buff = std::make_unique<PlayTrackIPC>();
		buff->command = BaseIPC::CommandType::PlaySequence;
		buff->sequenceData = sequenceData.get();
		buff->length = dataLen;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(PlayTrackIPC), (u8 *)buff.get());
	}

	void SequencePlayer::LoadBank(unsigned int bankId) {
		if(bankId == loadedBankIndex) return;
		loadedBankIndex = bankId;

		auto &bankInfo = sdat->GetBankInfo(bankId);
		sassert(bankInfo, "No bank info");
		for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
			LoadWaveArchive(swarIndex, bankInfo->swars[swarIndex]);
		}
		sbnk = sdat->OpenBank(bankInfo);
		//printf("Loaded bank %s.\n", sdat->GetNameForBank(bankId).c_str());

		std::unique_ptr<LoadBankIPC> buff = std::make_unique<LoadBankIPC>();
		buff->command = BaseIPC::CommandType::LoadBank;
		buff->bank = sbnk.get();

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadBankIPC), (u8 *)buff.get());
	}

	void SequencePlayer::LoadWaveArchive(unsigned int slot, unsigned int archiveId) {
		auto &loadedArchive = loadedWaveArchives[slot];
		loadedArchive.Reset();

		if(archiveId >= 0xFFFF) {
			std::unique_ptr<LoadWaveArchiveIPC> buff = std::make_unique<LoadWaveArchiveIPC>();
			buff->command = BaseIPC::CommandType::LoadWaveArchive;
			buff->slot = slot;
			buff->archive = nullptr;
			fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)buff.get());
			return;
		}

		auto &info = sdat->GetWaveArchiveInfo(archiveId);
		LoadWaveArchiveData(slot, info);

		std::unique_ptr<LoadWaveArchiveIPC> buff = std::make_unique<LoadWaveArchiveIPC>();
		buff->command = BaseIPC::CommandType::LoadWaveArchive;
		buff->slot = slot;
		buff->archive = &loadedArchive;
		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)buff.get());
	}

	void SequencePlayer::LoadWaveArchiveData(unsigned int slot, const std::unique_ptr<WaveArchiveInfoRecord> &info) {
		auto &loadedArchive = loadedWaveArchives[slot];
		auto swar = sdat->OpenWaveArchive(info);
		auto waveCount = swar->GetWaveCount();
		loadedArchive.waves.reserve(waveCount);

		for(unsigned int waveIndex = 0; waveIndex < waveCount; ++waveIndex) {
			auto &info = swar->GetWaveMetaData(waveIndex);

			LoadedWave loadedWave = info;

			auto stream = swar->GetWaveData(info);
			auto dataLen = stream->getLength();
			loadedWave.waveData = malloc(dataLen);
			sassert(loadedWave.waveData, "Not enough ram for wave data!");
			auto readLen = stream->read((uint8_t *)loadedWave.waveData, dataLen);
			sassert(readLen == dataLen, "Only read %d/%d bytes!", readLen, dataLen);

			loadedArchive.waves.emplace_back(std::move(loadedWave));
		}
	}


	void SequencePlayer::SetVar(std::uint8_t var, std::int16_t val) {
		std::unique_ptr<SetVarIPC> buff = std::make_unique<SetVarIPC>();
		buff->command = BaseIPC::CommandType::SetVar;
		buff->var = var;
		buff->val = val;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)buff.get());
	}
	std::int16_t SequencePlayer::GetVar(std::uint8_t var) const {
		std::unique_ptr<GetVarIPC> buff = std::make_unique<GetVarIPC>();
		buff->command = BaseIPC::CommandType::GetVar;
		buff->var = var;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)buff.get());

		fifoWaitValue32(FIFO_NITRO_COMPOSER);

		return static_cast<std::int16_t> (fifoGetValue32(FIFO_NITRO_COMPOSER));

	}

	void SequencePlayer::SetMainVolume(std::uint8_t volume) {

		std::unique_ptr<SetMainVolumeIPC> buff = std::make_unique<SetMainVolumeIPC>();
		buff->command = BaseIPC::CommandType::SetMainVolume;
		buff->volume = volume;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)buff.get());
	}

}