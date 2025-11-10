#include "sequencePlayer.h"

#include <cassert>

namespace NitroComposer {

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

}