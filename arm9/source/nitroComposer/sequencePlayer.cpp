#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/arm9/cache.h>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

	SequencePlayer::SequencePlayer() : sdat(nullptr) {
		playerId = musicEngine.registerPlayer(this);
	}
	SequencePlayer::~SequencePlayer() {
		KillSequence();
		musicEngine.unregisterPlayer(this);
	}

	void SequencePlayer::SetSdat(const SDatFile *sdat) {
		if(this->sdat == sdat) return;

		sseq.reset();
		sbnk.reset();
		for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
			loadedWaveArchives[swarIndex].Reset();
		}

		this->sdat = sdat;
	}

	void SequencePlayer::PlaySequence(unsigned int sequenceId) {
		const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceId);
		sassert(sequenceInfo, "Unknown sequence %u", sequenceId);
		PlaySequence(sequenceInfo);
	}

	void SequencePlayer::PlaySequence(const std::string &sequenceName) {
		const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceName);
		sassert(sequenceInfo, "Unknown sequence \"%s\"", sequenceName.c_str());
		PlaySequence(sequenceInfo);
	}

	void SequencePlayer::PlaySequence(const std::unique_ptr<SequenceInfoRecord> &sequenceInfo) {
		LoadBank(sequenceInfo->bankId);
		sseq = sdat->OpenSequence(sequenceInfo);

		auto &player = sdat->GetPlayerInfo(sequenceInfo->player);

		auto stream = sseq->GetCommandStream();
		size_t dataLen = stream->getLength();

		sequenceData = std::make_unique<std::uint8_t[]>(dataLen);
		auto readLen = stream->read(sequenceData.get(), dataLen);

		sassert(readLen == dataLen, "failed reading sequence data");

		PlayTrackIPC buff;
		buff.command = BaseIPC::CommandType::PlaySequence;
		buff.playerId = playerId;
		buff.sequenceData = sequenceData.get();
		buff.startPos = 0;
		buff.length = dataLen;
		buff.channelMask = player->channelMask;
		buff.sequenceVolume = sequenceInfo->vol;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(PlayTrackIPC), (u8 *)&buff);
		assert(success);

		isPlaying = true;
	}

	void SequencePlayer::AbortSequence() {
		SequencePlayerIPC buff;
		buff.command = BaseIPC::CommandType::StopSequence;
		buff.playerId = playerId;
		
		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequencePlayerIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			fifoGetValue32(FIFO_NITRO_COMPOSER);
		}

		isPlaying = false;
	}

	void SequencePlayer::KillSequence() {
		SequencePlayerIPC buff;
		buff.command = BaseIPC::CommandType::KillSequence;
		buff.playerId = playerId;

		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequencePlayerIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			fifoGetValue32(FIFO_NITRO_COMPOSER);
		}

		isPlaying = false;
	}

	void SequencePlayer::sequenceEnded() {
		puts("Sequence ended");
		isPlaying = false;
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

		LoadWaveFormsForCurrentBank();

		LoadBankIPC buff;
		buff.command = BaseIPC::CommandType::LoadBank;
		buff.playerId = playerId;
		buff.bank = sbnk.get();

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadBankIPC), (u8 *)&buff);
		assert(success);
	}

	void SequencePlayer::LoadWaveArchive(unsigned int archiveSlot, std::uint16_t archiveId) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];

		if(loadedArchive.archiveId == archiveId) return;

		loadedArchive.Reset();

		if(archiveId >= 0xFFFF) {
			LoadWaveArchiveIPC buff;
			buff.command = BaseIPC::CommandType::LoadWaveArchive;
			buff.playerId = playerId;
			buff.slot = archiveSlot;
			buff.archive = nullptr;
			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)&buff);
			assert(success);
			return;
		}

		auto &info = sdat->GetWaveArchiveInfo(archiveId);
		LoadWaveArchiveData(archiveSlot, info);
		loadedArchive.archiveId = archiveId;

		{
			LoadWaveArchiveIPC buff;
			buff.command = BaseIPC::CommandType::LoadWaveArchive;
			buff.playerId = playerId;
			buff.slot = archiveSlot;
			buff.archive = &loadedArchive;
			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)&buff);
			assert(success);
		}

		printf("Loaded archive %d \"%s\"\n", archiveId, sdat->GetNameForWaveArchive(archiveId).c_str());
	}

	void SequencePlayer::LoadWaveArchiveData(unsigned int archiveSlot, const std::unique_ptr<WaveArchiveInfoRecord> &info) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];
		auto &swar = swars[archiveSlot] = std::move(sdat->OpenWaveArchive(info));
		auto waveCount = swar->GetWaveCount();
		loadedArchive.waves.reserve(waveCount);

		for(unsigned int waveIndex = 0; waveIndex < waveCount; ++waveIndex) {
			auto &info = swar->GetWaveMetaData(waveIndex);

			LoadedWave loadedWave = info;
			loadedWave.waveData = nullptr;

			loadedArchive.waves.emplace_back(std::move(loadedWave));
		}
	}

	void SequencePlayer::LoadWaveArchiveWaveForm(unsigned int archiveSlot, std::uint16_t waveIndex) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];
		auto &swar = swars[archiveSlot];
		auto &loadedWave = loadedArchive.waves.at(waveIndex);

		if(loadedWave.waveData != nullptr) return;

		auto &info = swar->GetWaveMetaData(waveIndex);

		auto stream = swar->GetWaveData(info);
		auto dataLen = info.GetDataSize();
		loadedWave.waveData = malloc(dataLen);
		sassert(loadedWave.waveData, "Not enough ram for wave data! %i", dataLen);
		auto readLen = stream->read((uint8_t *)loadedWave.waveData, dataLen);
		sassert(readLen == dataLen, "Only read %d/%d bytes!", readLen, dataLen);

		DC_FlushRange(loadedWave.waveData, dataLen);
	}

	void SequencePlayer::UnloadWaveArchiveWaveForms(unsigned int archiveSlot) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];
		for(auto waveItr = loadedArchive.waves.begin(); waveItr < loadedArchive.waves.end(); ++waveItr) {
			auto &loadedWave = *waveItr;
			free(loadedWave.waveData);
			loadedWave.waveData = nullptr;
		}
	}

	void SequencePlayer::LoadWaveFormsForCurrentBank() {
		for(auto instItr = sbnk->instruments.cbegin(); instItr != sbnk->instruments.cend(); ++instItr) {
			auto inst = instItr->get();
			LoadWaveFormForInstrument(inst);
		}
	}

	void SequencePlayer::LoadWaveFormForInstrument(InstrumentBank::BaseInstrument *inst) {
		if(!inst) return;

		switch(inst->type) {
		case InstrumentBank::InstrumentType::Pulse:
		case InstrumentBank::InstrumentType::Noise:
		case InstrumentBank::InstrumentType::Null:
			break;
		case InstrumentBank::InstrumentType::PCM:
			LoadWaveFormForInstrument(static_cast<InstrumentBank::PCMInstrument *>(inst));
			break;
		case InstrumentBank::InstrumentType::Split:
			LoadWaveFormForInstrument(static_cast<InstrumentBank::SplitInstrument *>(inst));
			break;
		case InstrumentBank::InstrumentType::Drumkit:
			LoadWaveFormForInstrument(static_cast<InstrumentBank::Drumkit *>(inst));
			break;
		default:
			sassert(0, "Unknown instrument type %i", (int)inst->type);
		}
	}

	void SequencePlayer::LoadWaveFormForInstrument(InstrumentBank::PCMInstrument *inst) {
		LoadWaveArchiveWaveForm(inst->archive, inst->wave);
	}

	void SequencePlayer::LoadWaveFormForInstrument(InstrumentBank::SplitInstrument *split) {
		for(unsigned int range = 0; range < InstrumentBank::SplitInstrument::regionCount; ++range) {
			LoadWaveFormForInstrument(split->subInstruments[range].get());

			if(split->regions[range] == 127) break;
		}
	}

	void SequencePlayer::LoadWaveFormForInstrument(InstrumentBank::Drumkit *drums) {
		for(auto instItr = drums->subInstruments.cbegin(); instItr != drums->subInstruments.cend(); ++instItr) {
			InstrumentBank::BaseInstrument *inst = instItr->get();
			LoadWaveFormForInstrument(inst);
		}
	}

	void SequencePlayer::SetVar(std::uint8_t var, std::int16_t val) {
		SetVarIPC buff;
		buff.command = BaseIPC::CommandType::SetVar;
		buff.playerId = playerId;
		buff.var = var;
		buff.val = val;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)&buff);
		assert(success);
	}
	std::int16_t SequencePlayer::GetVar(std::uint8_t var) const {
		GetVarIPC buff;
		buff.command = BaseIPC::CommandType::GetVar;
		buff.playerId = playerId;
		buff.var = var;

		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(GetVarIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			return static_cast<std::int16_t> (fifoGetValue32(FIFO_NITRO_COMPOSER));
		}

	}

	void SequencePlayer::SetTrackMute(std::uint8_t trackId, bool mute) {
		MuteTrackIPC buff;
		buff.command = BaseIPC::CommandType::SetVar;
		buff.playerId = playerId;
		buff.trackId = trackId;
		buff.mute = mute;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(MuteTrackIPC), (u8 *)&buff);
		assert(success);
	}

}