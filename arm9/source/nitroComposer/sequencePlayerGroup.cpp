#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/arm9/cache.h>
#include <algorithm>

namespace NitroComposer {
	SequencePlayerGroup::SequencePlayerGroup() : loadedSequenceId(0xFFFFFFFF), loadedSequenceArchiveId(0xFFFFFFFF), loadedBankIndex(0xFFFFFFFF) {}

	SequencePlayerGroup::~SequencePlayerGroup() {
		assert(players.empty());

		UnloadSequenceData();
	}

	void SequencePlayerGroup::KillAllSequences() {
		for(SequencePlayer *player : players) {
			player->KillSequence();
		}
	}

	void SequencePlayerGroup::SetSdat(const SDatFile *sdat) {
		if(this->sdat == sdat) return;

		UnloadSequenceData();
		sbnk.reset();
		for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
			loadedWaveArchives[swarIndex].Reset();
		}

		this->sdat = sdat;
	}

	void SequencePlayerGroup::LoadSequence(unsigned int sequenceId) {
		assert(sequenceId != 0xFFFFFFFF);
		if(sequenceId == loadedSequenceId) return;

		const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceId);
		sassert(sequenceInfo, "Unknown sequence %u", sequenceId);

		LoadSequence(sequenceInfo);
		loadedSequenceId = sequenceId;
	}

	void SequencePlayerGroup::LoadSequence(const std::string &sequenceName) {
		unsigned int sequenceId = sdat->GetNamedSequenceIndex(sequenceName);
		sassert(sequenceId != 0xFFFFFFFF, "Unknown sequence \"%s\"", sequenceName.c_str());

		LoadSequence(sequenceId);
	}

	const std::unique_ptr<SequenceInfoRecord> &SequencePlayerGroup::GetLoadedSequenceInfo() const {
		assert(loadedSequenceId != 0xFFFFFFFF);
		return sdat->GetSequenceInfo(loadedSequenceId);
	}

	void SequencePlayerGroup::LoadSequence(const std::unique_ptr<SequenceInfoRecord> &sequenceInfo) {
		UnloadSequenceData();
		LoadBank(sequenceInfo->bankId);
		std::construct_at(&sseq, sdat->OpenSequence(sequenceInfo));

		auto stream = sseq->GetCommandStream();
		sequenceDataSize = stream->getLength();

		sequenceData = std::make_unique<std::uint8_t[]>(sequenceDataSize);
		auto readLen = stream->read(sequenceData.get(), sequenceDataSize);

		sassert(readLen == sequenceDataSize, "failed reading sequence data");
	}

	void SequencePlayerGroup::LoadBank(unsigned int bankId) {
		if(bankId == loadedBankIndex) return;
		loadedBankIndex = bankId;

		KillAllSequences();

		auto &bankInfo = sdat->GetBankInfo(bankId);
		sassert(bankInfo, "No bank info");
		for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
			LoadWaveArchive(swarIndex, bankInfo->swars[swarIndex]);
		}
		sbnk = sdat->OpenBank(bankInfo);
		//printf("Loaded bank %s.\n", sdat->GetNameForBank(bankId).c_str());

		try {
			LoadWaveFormsForCurrentBank();
		} catch(const std::bad_alloc &) {
			UnloadWaveArchiveWaveForms(0);
			UnloadWaveArchiveWaveForms(1);
			UnloadWaveArchiveWaveForms(2);
			UnloadWaveArchiveWaveForms(3);

			try {
				LoadWaveFormsForCurrentBank();
			} catch(const std::bad_alloc &) {
				sassert(0, "Out of waveform memory for bank %u", bankId);
			}
		}

		sendLoadBankIPC(sbnk.get());
	}

	void SequencePlayerGroup::LoadWaveArchive(unsigned int archiveSlot, std::uint16_t archiveId) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];

		if(loadedArchive.archiveId == archiveId) return;

		loadedArchive.Reset();

		if(archiveId >= 0xFFFF) {
			sendLoadWaveArchiveIPC(archiveSlot, nullptr);
			return;
		}

		auto &info = sdat->GetWaveArchiveInfo(archiveId);
		LoadWaveArchiveData(archiveSlot, info);
		loadedArchive.archiveId = archiveId;

		sendLoadWaveArchiveIPC(archiveSlot, &loadedArchive);

		printf("Loaded archive %d \"%s\"\n", archiveId, sdat->GetNameForWaveArchive(archiveId).c_str());
	}


	void SequencePlayerGroup::LoadWaveArchiveData(unsigned int archiveSlot, const std::unique_ptr<WaveArchiveInfoRecord> &info) {
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

	void SequencePlayerGroup::LoadWaveArchiveWaveForm(unsigned int archiveSlot, std::uint16_t waveIndex) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];
		auto &swar = swars[archiveSlot];
		auto &loadedWave = loadedArchive.waves.at(waveIndex);

		if(loadedWave.waveData != nullptr) return;

		auto &info = swar->GetWaveMetaData(waveIndex);

		auto stream = swar->GetWaveData(info);
		auto dataLen = info.GetDataSize();
		loadedWave.waveData = malloc(dataLen);
		if(!loadedWave.waveData) throw std::bad_alloc();
		auto readLen = stream->read((uint8_t *)loadedWave.waveData, dataLen);
		sassert(readLen == dataLen, "Only read %d/%d bytes!", readLen, dataLen);

		DC_FlushRange(loadedWave.waveData, dataLen);
	}

	void SequencePlayerGroup::UnloadWaveArchiveWaveForms(unsigned int archiveSlot) {
		auto &loadedArchive = loadedWaveArchives[archiveSlot];
		for(auto waveItr = loadedArchive.waves.begin(); waveItr < loadedArchive.waves.end(); ++waveItr) {
			auto &loadedWave = *waveItr;
			free(loadedWave.waveData);
			loadedWave.waveData = nullptr;
		}
	}

	void SequencePlayerGroup::RegisterPlayer(SequencePlayer *player) {
		assert(player);
		assert(std::find(players.begin(), players.end(), player) == players.end());
		players.push_back(player);
	}

	void SequencePlayerGroup::UnregisterPlayer(SequencePlayer *player) {
		assert(player);
		auto itr = std::find(players.begin(), players.end(), player);
		assert(itr != players.end());
		players.erase(itr);
	}

	void SequencePlayerGroup::LoadWaveFormsForCurrentBank() {
		for(auto instItr = sbnk->instruments.cbegin(); instItr != sbnk->instruments.cend(); ++instItr) {
			auto inst = instItr->get();
			LoadWaveFormForInstrument(inst);
		}
	}

	bool SequencePlayerGroup::hasLoadedSequenceData() const {
		return loadedSequenceArchiveId != 0xFFFFFFFF || loadedSequenceId != 0xFFFFFFFF;
	}

	void SequencePlayerGroup::UnloadSequenceData() {

		for(SequencePlayer *player : players) {
			assert(!player->IsPlaying());
		}

		if(sequenceData) {
			sequenceData.reset();
			sequenceDataSize = 0;
		}

		if(loadedSequenceId != 0xFFFFFFFF) {
			loadedSequenceId = 0xFFFFFFFF;
			sseq.~unique_ptr();
		} else if(loadedSequenceArchiveId != 0xFFFFFFFF) {
			loadedSequenceArchiveId = 0xFFFFFFFF;
			ssar.~unique_ptr();
		}
	}

	void SequencePlayerGroup::LoadWaveFormForInstrument(InstrumentBank::BaseInstrument *inst) {
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

	void SequencePlayerGroup::LoadWaveFormForInstrument(InstrumentBank::PCMInstrument *inst) {
		LoadWaveArchiveWaveForm(inst->archive, inst->wave);
	}

	void SequencePlayerGroup::LoadWaveFormForInstrument(InstrumentBank::SplitInstrument *split) {
		for(unsigned int range = 0; range < InstrumentBank::SplitInstrument::regionCount; ++range) {
			LoadWaveFormForInstrument(split->subInstruments[range].get());

			if(split->regions[range] == 127) break;
		}
	}

	void SequencePlayerGroup::LoadWaveFormForInstrument(InstrumentBank::Drumkit *drums) {
		for(auto instItr = drums->subInstruments.cbegin(); instItr != drums->subInstruments.cend(); ++instItr) {
			InstrumentBank::BaseInstrument *inst = instItr->get();
			LoadWaveFormForInstrument(inst);
		}
	}

	void SequencePlayerGroup::sendLoadBankIPC(const NitroComposer::InstrumentBank *loadedBank) {
		for(SequencePlayer *player : players) {
			player->sendLoadBankIPC(loadedBank);
		}
	}

	void SequencePlayerGroup::sendLoadWaveArchiveIPC(unsigned int archiveSlot, const NitroComposer::LoadedWaveArchive *loadedArchive) {
		for(SequencePlayer *player : players) {
			player->sendLoadWaveArchiveIPC(archiveSlot, loadedArchive);
		}
	}
}