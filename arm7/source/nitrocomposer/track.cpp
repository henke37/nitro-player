#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm7/console.h>

namespace NitroComposer {

	SequencePlayer::Track::Track() {}

	void SequencePlayer::Track::Init(SequencePlayer *player) {
		assert(player);
		this->player = player;

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
	}

	void SequencePlayer::Track::Tick() {
		if(!isPlaying) return;

		if(waitCounter) {
			--waitCounter;
			return;
		}

		while(!waitCounter) {
			ExecuteNextCommand();
		}
	}

	void SequencePlayer::Track::StartPlaying(std::uint32_t offset) {
		this->isPlaying = true;
		SetNextCommand(offset);
	}

	void SequencePlayer::Track::StopPlaying() {
		this->isPlaying = false;
	}


	void SequencePlayer::Track::NoteOn(std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		note += transpose;
		if(tieMode) {
			consolePrintf("Tie-Note on %d,%d\n", note, velocity);
			//NoteOnTie(note, velocity);
		} else {
			consolePrintf("Note on %d,%d,%d\n", note, velocity, length);
			NoteOnReal(note, velocity, length);
		}
		consoleFlush();
		lastPlayedNote = note;
		if(noteWait) {
			this->waitCounter = length;
		}
	}

	void SequencePlayer::Track::NoteOnReal(std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		const InstrumentBank::LeafInstrument *noteInstrument = ResolveInstrumentForNote(note);

		if(!noteInstrument) return;

		auto voiceIndex = this->player->FindFreeVoice(noteInstrument->type);
		if(voiceIndex < 0) return;

		auto &voice = this->player->voices[voiceIndex];

		voice.StartNote(this, noteInstrument, note, velocity, length);
	}

	void SequencePlayer::Track::SetNextCommand(std::uint32_t offset) {
		this->nextCommand=this->player->sequenceData + offset;
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

	void SequencePlayer::Track::SetInstrument(unsigned int instrumentId) {
		currentInstrument = player->bank->instruments.at(instrumentId).get();
	}
}