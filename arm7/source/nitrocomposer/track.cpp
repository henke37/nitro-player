#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm7/console.h>
#include <algorithm>

namespace NitroComposer {

	SequencePlayer::Track::Track() : isPlaying(false), muted(false) {}

	void SequencePlayer::Track::Init(PlayingSequence *sequence) {
		assert(sequence);
		this->sequence = sequence;

		Reset();
	}

	void SequencePlayer::Track::Reset() {
		isPlaying = false;
		noteWait = true;
		tieMode = false;
		waitCounter = 0;
		priority = 64;
		lastComparisonResult = true;
		attack = 0xFF;
		decay = 0xFF;
		sustain = 0xFF;
		release = 0xFF;
		volume = 127;
		expression = 127;
		transpose = 0;
		pan = 0;

		portamento = false;
		lastPlayedNote = 60;
		portaTime = 0;

		pitchBend = 0;
		pitchBendRange = 2;

		modMode = ModulationMode::Vibrato;
		modDepth = 0;
		modDelay = 0;
		modSpeed = 0;

		stackPointer = 0;
		nextCommand = nullptr;
	}

	void SequencePlayer::Track::Tick() {
		if(!isPlaying) return;

		if(waitCounter) {
			--waitCounter;
			if(waitCounter) return;
		}

		while((waitCounter==0) && isPlaying) {
			ExecuteNextCommand();
		}
	}

	void SequencePlayer::Track::StartPlaying(std::ptrdiff_t offset) {
		this->isPlaying = true;
		SetNextCommand(offset);
	}

	void SequencePlayer::Track::StopPlaying() {
		if(!this->isPlaying) return;
		this->isPlaying = false;
		sequence->stoppedPlaying(this);
	}

	std::uint8_t SequencePlayer::Track::GetTransposedNote(std::uint8_t note) const {
		return std::clamp((int)note + transpose, 0, 127);
	}

	std::uint8_t SequencePlayer::Track::GetPriority() const {
		return std::clamp((int)priority + sequence->priority, 0, 256);
	}

	void SequencePlayer::Track::NoteOn(std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		if(!muted) {
			note = GetTransposedNote(note);
			if(tieMode) {
				consolePrintf("#%d Tie-Note on %d,%d\n", GetId(), note, velocity);
				//NoteOnTie(note, velocity);
			} else {
				consolePrintf("#%d Note on %d,%d,%d\n", GetId(), note, velocity, length);
				NoteOnReal(note, velocity, length);
			}
			consoleFlush();
			lastPlayedNote = note;
		}
		if(noteWait) {
			this->waitCounter = length;
		}
	}

	void SequencePlayer::Track::NoteOnReal(std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		const InstrumentBank::LeafInstrument *noteInstrument = ResolveInstrumentForNote(note);

		if(!noteInstrument) return;

		Voice *voice=sequence->allocateVoice(noteInstrument->type, this);

		voice->StartNote(this, noteInstrument, note, velocity, length);
	}

	void SequencePlayer::Track::SetNextCommand(std::ptrdiff_t offset) {
		assert((size_t)offset < this->sequence->sequenceDataLength);
		this->nextCommand=this->sequence->sequenceData + offset;
	}

	const InstrumentBank::LeafInstrument *SequencePlayer::Track::ResolveInstrumentForNote(std::uint8_t note) const {
		switch(this->currentInstrument->type) {
			case InstrumentBank::InstrumentType::Null:
				return nullptr;
			case InstrumentBank::InstrumentType::PCM:
			case InstrumentBank::InstrumentType::Pulse:
			case InstrumentBank::InstrumentType::Noise:
				return static_cast<const InstrumentBank::LeafInstrument *>(currentInstrument);

			case InstrumentBank::InstrumentType::Drumkit: {
				auto drums = static_cast<const InstrumentBank::Drumkit *>(currentInstrument);
				if(note > drums->maxNote || note < drums->minNote) return nullptr;

				std::uint8_t offset = note - drums->minNote;
				return static_cast<const InstrumentBank::LeafInstrument *>(drums->subInstruments.at(offset).get());
			} break;

			case InstrumentBank::InstrumentType::Split: {
				auto split = static_cast<const InstrumentBank::SplitInstrument *>(currentInstrument);
				for(unsigned int region = 0; region < InstrumentBank::SplitInstrument::regionCount; ++region) {
					if(note <= split->regions[region]) {
						return static_cast<const InstrumentBank::LeafInstrument *>(split->subInstruments[region].get());
					}
				}
			} break;
		}
		assert(0);
		return nullptr;
	}

	std::uint8_t SequencePlayer::Track::GetId() const {
		return sequence->IdForTrack(this);
	}

	void SequencePlayer::Track::ReleaseAllVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = sequencePlayer.voices[voiceIndex];
			if(voice.GetTrack() != this) continue;
			voice.Release();
		}
	}

	void SequencePlayer::Track::KillAllVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = sequencePlayer.voices[voiceIndex];
			if(voice.GetTrack() != this) continue;
			voice.Kill();
		}
	}

	void SequencePlayer::Track::SetInstrument(unsigned int instrumentId) {
		currentInstrument = sequence->bank->instruments.at(instrumentId).get();
	}
	void SequencePlayer::Track::SetMute(MuteMode mode) {
		switch(mode) {
		case MuteMode::Clear:
			muted = false;
			break;
		case MuteMode::MuteAndRelease:
		case MuteMode::MuteAndKill:
			if(mode == MuteMode::MuteAndRelease) {
				ReleaseAllVoices();
			} else {
				KillAllVoices();
			}
			[[fallthrough]];
		case MuteMode::Mute:
			muted = true;
			break;
		}
	}
}