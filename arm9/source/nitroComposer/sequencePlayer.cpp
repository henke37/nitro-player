#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/system.h>
#include <nds/arm9/cache.h>

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

	void SequencePlayer::LoadWaveArchive(unsigned int archiveSlot, std::uint16_t archiveId) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];

		if(loadedArchive.archiveId == archiveId) return;

		loadedArchive.Reset();

		if(archiveId >= 0xFFFF) {
			std::unique_ptr<LoadWaveArchiveIPC> buff = std::make_unique<LoadWaveArchiveIPC>();
			buff->command = BaseIPC::CommandType::LoadWaveArchive;
			buff->slot = archiveSlot;
			buff->archive = nullptr;
			fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)buff.get());
			return;
		}

		auto &info = sdat->GetWaveArchiveInfo(archiveId);
		LoadWaveArchiveData(archiveSlot, info);
		loadedArchive.archiveId = archiveId;

		std::unique_ptr<LoadWaveArchiveIPC> buff = std::make_unique<LoadWaveArchiveIPC>();
		buff->command = BaseIPC::CommandType::LoadWaveArchive;
		buff->slot = archiveSlot;
		buff->archive = &loadedArchive;
		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)buff.get());

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
		for(std::uint8_t note = drums->minNote; note <= drums->maxNote; ++note) {
			auto inst = drums->subInstruments[note - (drums->minNote)].get();
			LoadWaveFormForInstrument(inst);
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