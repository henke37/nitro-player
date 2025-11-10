#include "sequencePlayer.h"

#include <nds/timers.h>
#include <nds/interrupts.h>
#include <nds/arm7/console.h>
#include <nds/system.h>
#include <nds/arm7/audio.h>

#include <cassert>

namespace NitroComposer {

	static const uint8_t pcmChannels[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const uint8_t psgChannels[] = { 8, 9, 10, 11, 12, 13 };
	static const uint8_t noiseChannels[] = { 14, 15 };

	SequencePlayer sequencePlayer;

	void SequencePlayer::Init() {
		for(unsigned int var = 0; var < globalVariableCount; ++var) {
			globalVariables[var] = -1;
		}

		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			voices[voiceIndex].Init(voiceIndex);
		}

		playingSequence.Init();

		externalChannelReservations = 0;
		allowedChannels = 0xFF;

		setupFifo();
	}

	void SequencePlayer::PlayingSequence::Init() {
		ResetLocalVars();

		for(unsigned int trackIndex = 0; trackIndex < voiceCount; ++trackIndex) {
			tracks[trackIndex].Init(this);
		}

		for(unsigned int waveArchiveIndex = 0; waveArchiveIndex < numWaveArchs; ++waveArchiveIndex) {
			waveArchs[waveArchiveIndex] = nullptr;
		}
	}

	void SequencePlayer::PlayingSequence::ResetLocalVars() {
		for(unsigned int var = 0; var < localVariableCount; ++var) {
			localVariables[var] = -1;
		}
	}

	void SequencePlayer::PlayingSequence::PlaySequence(const std::uint8_t *sequenceData) {
		AbortSequence();

		ResetLocalVars();

		this->sequenceData = sequenceData;
		this->tempo = 120;

		StartTrack(0, 0);
	}

	void SequencePlayer::PlayingSequence::AbortSequence() {
		for(unsigned int trackIndex = 0; trackIndex < voiceCount; ++trackIndex) {
			tracks[trackIndex].StopPlaying();
		}
	}

	void SequencePlayer::PlayingSequence::StartTrack(std::uint8_t trackId, std::uint32_t offset) {
		assert(trackId < trackCount);
		auto &track = tracks[trackId];
		track.StartPlaying(offset);
	}

	void SequencePlayer::PlayingSequence::SetVar(std::uint8_t var, std::int16_t val) {
		if(var < localVariableCount) {
			localVariables[var] = val;
		} else {
			var -= localVariableCount;
			assert(var < globalVariableCount);
			sequencePlayer.globalVariables[var] = val;
		}
	}

	std::int16_t SequencePlayer::PlayingSequence::GetVar(std::uint8_t var) const {
		if(var < localVariableCount) {
			return localVariables[var];
		} else {
			var -= localVariableCount;
			assert(var < globalVariableCount);
			return sequencePlayer.globalVariables[var];
		}
	}

	signed int SequencePlayer::FindFreeVoice(InstrumentBank::InstrumentType type, const PlayingSequence *) {
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

	const LoadedWave &SequencePlayer::PlayingSequence::GetWave(unsigned int archiveSlot, unsigned int waveIndex) {
		assert(archiveSlot < numWaveArchs);
		assert(this->waveArchs[archiveSlot]);
		assert(waveIndex < this->waveArchs[archiveSlot]->waves.size());
		return this->waveArchs[archiveSlot]->waves.at(waveIndex);
	}


	bool SequencePlayer::isVoiceAllowed(std::uint8_t voiceIndex) const {
		if(!(allowedChannels & BIT(voiceIndex)) && allowedChannels) return false;
		if((externalChannelReservations & BIT(voiceIndex)) && externalChannelReservations) return false;

		return true;
	}

	SequencePlayer::Voice *SequencePlayer::PlayingSequence::allocateVoice(InstrumentBank::InstrumentType type) {
		auto voiceIndex = sequencePlayer.FindFreeVoice(type, this);
		if(voiceIndex < 0) return nullptr;

		Voice *voice = &sequencePlayer.voices[voiceIndex];
		voice->Kill();
		voices[voiceIndex] = voice;

		return voice;
	}

	void SequencePlayer::setupTimer() {
		timerStart(LIBNDS_DEFAULT_TIMER_MUSIC, ClockDivider_64, -2728, ISR);
	}

	void SequencePlayer::ISR() {
		sequencePlayer.Update();
	}

	void SequencePlayer::Update() {
		playingSequence.Update();
		UpdateVoices();
	}

	void SequencePlayer::PlayingSequence::Update() {
		while(tempoTimer >= 240) {
			tempoTimer -= 240;
			TickVoices();
			TickTracks();
		}
		tempoTimer += tempo;
	}

	void SequencePlayer::PlayingSequence::TickVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			if(!voice) continue;
			voice->Tick();
		}
	}

	void SequencePlayer::UpdateVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			voice.Update();
		}
	}

	void SequencePlayer::PlayingSequence::TickTracks() {
		for(unsigned int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
			auto &track = tracks[trackIndex];
			track.Tick();
		}
	}

}