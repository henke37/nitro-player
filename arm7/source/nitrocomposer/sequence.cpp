#include "sequencePlayer.h"

#include <nds/arm7/console.h>

#include <cassert>

namespace NitroComposer {

	void SequencePlayer::PlayingSequence::Init() {
		ResetLocalVars();

		allowedChannels = 0xFFFF;

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

	void SequencePlayer::PlayingSequence::ResetTracks() {
		for(unsigned int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
			tracks[trackIndex].Reset();
		}
	}

	void SequencePlayer::PlayingSequence::PlaySequence(const std::uint8_t *sequenceData, size_t length, std::ptrdiff_t startPos) {
		AbortSequence(true);

		ResetLocalVars();
		ResetTracks();

		this->sequenceData = sequenceData;
		this->sequenceDataLength = length;
		this->tempo = 120;

		StartTrack(0, startPos);
	}

	void SequencePlayer::PlayingSequence::AbortSequence(bool killVoices) {
		for(unsigned int trackIndex = 0; trackIndex < voiceCount; ++trackIndex) {
			tracks[trackIndex].StopPlaying();
		}
		if(killVoices) {
			KillAllVoices();
		}
	}

	void SequencePlayer::PlayingSequence::StartTrack(std::uint8_t trackId, std::ptrdiff_t offset) {
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

	const LoadedWave &SequencePlayer::PlayingSequence::GetWave(unsigned int archiveSlot, unsigned int waveIndex) {
		assert(archiveSlot < numWaveArchs);
		assert(this->waveArchs[archiveSlot]);
		assert(waveIndex < this->waveArchs[archiveSlot]->waves.size());
		return this->waveArchs[archiveSlot]->waves.at(waveIndex);
	}

	SequencePlayer::Voice *SequencePlayer::PlayingSequence::allocateVoice(InstrumentBank::InstrumentType type) {
		auto voiceIndex = sequencePlayer.FindFreeVoice(type, this);
		if(voiceIndex < 0) return nullptr;

		Voice *voice = &sequencePlayer.voices[voiceIndex];
		voice->Kill();
		voices[voiceIndex] = voice;

		return voice;
	}

	void SequencePlayer::PlayingSequence::stoppedPlaying(Track *track) {
		for(unsigned int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
			auto &track = tracks[trackIndex];
			if(track.GetIsPlaying()) return;
		}
		ReleaseAllVoices();
		sequencePlayer.sendFifoSequenceStatus(*this);
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

	void SequencePlayer::PlayingSequence::TickTracks() {
		for(unsigned int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
			auto &track = tracks[trackIndex];
			track.Tick();
		}
	}

	void SequencePlayer::PlayingSequence::ReleaseAllVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			if(!voice) continue;
			voice->Release();
		}
	}

	void SequencePlayer::PlayingSequence::KillAllVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			if(!voice) continue;
			voice->Kill();
		}
	}

}