#include "sequencePlayer.h"

#include <nds/fifocommon.h>
#include <nds/timers.h>
#include <nds/interrupts.h>
#include <nds/arm7/console.h>
#include <nds/system.h>
#include <nds/arm7/audio.h>

#include <cassert>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

	static const uint8_t pcmChannels[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const uint8_t psgChannels[] = { 8, 9, 10, 11, 12, 13 };
	static const uint8_t noiseChannels[] = { 14, 15 };

	SequencePlayer sequencePlayer;

	const size_t fifoBuffSize = 32;

	void SequencePlayer::Init() {
		for(unsigned int var = 0; var < numVariables; ++var) {
			variables[var] = -1;
		}

		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			voices[voiceIndex].Init(voiceIndex);
		}

		for(unsigned int trackIndex = 0; trackIndex < voiceCount; ++trackIndex) {
			tracks[trackIndex].Init(this);
		}

		for(unsigned int waveArchiveIndex = 0; waveArchiveIndex < numWaveArchs; ++waveArchiveIndex) {
			waveArchs[waveArchiveIndex] = nullptr;
		}

		externalChannelReservations = 0;
		allowedChannels = 0xFF;

		setupFifo();
	}

	void SequencePlayer::SetVar(std::uint8_t var, std::int16_t val) {
		variables[var] = val;
	}

	std::int16_t SequencePlayer::GetVar(std::uint8_t var) const {
		return variables[var];
	}

	void SequencePlayer::PlaySequence(const std::uint8_t *sequenceData) {
		AbortSequence();

		this->sequenceData = sequenceData;
		this->tempo = 120;

		StartTrack(0, 0);
	}

	void SequencePlayer::AbortSequence() {
		for(unsigned int trackIndex = 0; trackIndex < voiceCount; ++trackIndex) {
			tracks[trackIndex].StopPlaying();
		}
	}

	void SequencePlayer::StartTrack(std::uint8_t trackId, std::uint32_t offset) {
		assert(trackId < trackCount);
		auto &track = tracks[trackId];
		track.StartPlaying(offset);
	}



	signed int SequencePlayer::FindFreeVoice(InstrumentBank::InstrumentType type) {
		size_t channelCount;
		const uint8_t *channelList;
		switch(type) {
		case InstrumentBank::InstrumentType::PCM:
			channelCount = 16;
			channelList = pcmChannels;
			break;
		case InstrumentBank::InstrumentType::Pulse:
			channelCount = 6;
			channelList = psgChannels;
			break;
		case InstrumentBank::InstrumentType::Noise:
			channelCount = 2;
			channelList = noiseChannels;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
		default:
			assert(0);
		}

		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			if(!isVoiceAllowed(voiceIndex)) continue;
			auto &voice = voices[voiceIndex];
			if(voice.state == VoiceState::Free) return voiceIndex;
		}

		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			if(!isVoiceAllowed(voiceIndex)) continue;
			auto &voice = voices[voiceIndex];
			if(voice.state == VoiceState::Releasing) return voiceIndex;
		}

		//TODO: voice stealing

		return -1;
	}

	const LoadedWave &SequencePlayer::GetWave(unsigned int archiveSlot, unsigned int waveIndex) {
		assert(archiveSlot < numWaveArchs);
		assert(this->waveArchs[archiveSlot]);
		assert(waveIndex < this->waveArchs[archiveSlot]->waves.size());
		return this->waveArchs[archiveSlot]->waves.at(waveIndex);
	}

	void SequencePlayer::setupFifo() {
		fifoSetDatamsgHandler(FIFO_NITRO_COMPOSER, fifoDatagramHandler, nullptr);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes, void *userdata) {
		sequencePlayer.fifoDatagramHandler(num_bytes);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes) {
		u8 fifoBuffer[fifoBuffSize];
		fifoGetDatamsg(FIFO_USER_01, fifoBuffSize, fifoBuffer);

		auto ipc = reinterpret_cast<BaseIPC *>(fifoBuffer);

		switch(ipc->command) {

		case BaseIPC::CommandType::PowerOn:
		{
			enableSound();

			setupTimer();
		} break;

		case BaseIPC::CommandType::SetVar:
		{
			SetVarIPC *setVarIpc = static_cast<SetVarIPC *>(ipc);
			SetVar(setVarIpc->var, setVarIpc->val);
		} break;

		case BaseIPC::CommandType::GetVar:
		{
			GetVarIPC *getVarIpc = static_cast<GetVarIPC *>(ipc);
			std::int16_t val = GetVar(getVarIpc->var);
			bool success = fifoSendValue32(FIFO_NITRO_COMPOSER, val);
			assert(success);
		} break;

		case BaseIPC::CommandType::SetTempo:
		{
			SetTempoIPC *setTempoIpc = static_cast<SetTempoIPC *>(ipc);
			this->tempo = setTempoIpc->tempo;
		} break;
		case BaseIPC::CommandType::SetMainVolume:
		{
			SetMainVolumeIPC *setMainVolumeIpc = static_cast<SetMainVolumeIPC *>(ipc);
			this->mainVolume = setMainVolumeIpc->volume;
			REG_MASTER_VOLUME = setMainVolumeIpc->volume;
		} break;

		case BaseIPC::CommandType::LoadBank:
		{
			LoadBankIPC *loadBankIpc = static_cast<LoadBankIPC *>(ipc);
			this->bank = loadBankIpc->bank;

			consolePrintf("Loaded %d instruments\n", this->bank->instruments.size());
			consoleFlush();
		} break;

		case BaseIPC::CommandType::LoadWaveArchive:
		{
			LoadWaveArchiveIPC *loadWaveArchiveIpc = static_cast<LoadWaveArchiveIPC *>(ipc);
			assert(loadWaveArchiveIpc->slot < numWaveArchs);
			this->waveArchs[loadWaveArchiveIpc->slot] = loadWaveArchiveIpc->archive;

			if(!loadWaveArchiveIpc->archive) break;

			consolePrintf("Loaded bank %d with %d waves\n", loadWaveArchiveIpc->slot, loadWaveArchiveIpc->archive->waves.size());
			consoleFlush();
		} break;

		case BaseIPC::CommandType::PlaySequence:
		{
			PlayTrackIPC *playTrackIpc = static_cast<PlayTrackIPC *>(ipc);
			PlaySequence(playTrackIpc->sequenceData);
		} break;

		case BaseIPC::CommandType::ReserveChannels:
		{
			ReserveChannelsIPC *reserveIpc = static_cast<ReserveChannelsIPC *>(ipc);
			externalChannelReservations = reserveIpc->reservations;
		}

		default:
			assert(0);
		}
	}

	void SequencePlayer::setupTimer() {
		timerStart(1, ClockDivider_64, -2728, ISR);
	}

	void SequencePlayer::ISR() {
		sequencePlayer.Tick();
	}

	bool SequencePlayer::isVoiceAllowed(std::uint8_t voiceIndex) const {
		if(!(allowedChannels & BIT(voiceIndex)) && allowedChannels) return false;
		if(!(externalChannelReservations & BIT(voiceIndex)) && externalChannelReservations) return false;

		return true;
	}

	void SequencePlayer::Tick() {

		while(tempoTimer >= 240) {
			tempoTimer -= 240;
			TickTracks();
		}
		tempoTimer += tempo;

		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			voice.Tick();
		}
	}

	void SequencePlayer::TickTracks() {
		for(unsigned int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
			auto &track = tracks[trackIndex];
			track.Tick();
		}
	}

}