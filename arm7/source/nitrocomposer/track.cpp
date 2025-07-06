#include "sequencePlayer.h"

#include <cassert>

namespace NitroComposer {

	SequencePlayer::Track::Track() :
		isPlaying(false),
		noteWait(true),
		tieMode(false),
		attack(0xFF),
		decay(0xFF),
		sustain(0xFF),
		release(0xFF) {}

	void SequencePlayer::Track::Init(SequencePlayer *player) {
		assert(player);
		this->player = player;
	}

	void SequencePlayer::Track::Tick() {
		if(!isPlaying) return;

		if(wait) {
			--wait;
			return;
		}

		while(!wait) {
			ExecuteNextCommand();
		}
	}

	void SequencePlayer::Track::ExecuteNextCommand() {}

	void SequencePlayer::Track::NoteOn(std::uint8_t note, unsigned int length) {
		if(tieMode) {
			NoteOnTie(note);
		} else {
			NoteOnReal(note, length);
		}
		if(noteWait) {
			this->wait = length;
		}
	}

	void SequencePlayer::Track::NoteOnReal(std::uint8_t note, unsigned int length) {
		const InstrumentBank::LeafInstrument *noteInstrument = ResolveInstrumentForNote(note);
		this->player->FindFreeVoice();
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
					if(note < split->regions[region]) {
						return static_cast<const InstrumentBank::LeafInstrument *>(split->subInstruments[region].get());
					}
				}
			} break;
		}
		assert(0);
		return nullptr;
	}

}