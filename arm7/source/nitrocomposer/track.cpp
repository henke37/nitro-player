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

	void SequencePlayer::Track::ExecuteNextCommand() {
		std::uint8_t command = readByteCommand();

		if(command < 0x80) {
			std::uint8_t velocity = readByteCommand();
			unsigned int length = readMidiVarLen();
			NoteOn(command, velocity, length);
			return;
		}

		switch(command) {
		case 0x80:
			wait = readMidiVarLen();
			break;
		case 0x81:
		{
			unsigned int programId = readMidiVarLen();
			SetInstrument(programId);
		} break;

		case 0xC7:
		{
			noteWait = readByteCommand()!=0;
		} break;

		case 0xE1:
		{
			player->tempo = readShortCommand();
		}break;

		default:
			skipCommandArgs(command);
			break;

		}
	}

	void SequencePlayer::Track::skipCommandArgs(std::uint8_t command) {
		if(command < 0x80) {
			++nextCommand;
			readMidiVarLen();
			return;
		}
		switch(command) {
		case 0x80:
			readMidiVarLen();
			break;
		case 0x81:
			readMidiVarLen();
			break;
		case 0x93:
			nextCommand += 4;
			break;
		case 0x94:
		case 0x95:
			nextCommand += 3;
			break;
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
			nextCommand += 3;
			break;
		case 0xC0:
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
			++nextCommand;
			break;
		case 0xE0:
		case 0xE1:
		case 0xE3:
			nextCommand += 2;
			break;
		case 0xFC:
		case 0xFD:
			break;
		case 0xFE:
			nextCommand += 2;
			break;
		case 0xFF:
			++nextCommand;
			break;
		default:
			assert(0);
		}
	}

	void SequencePlayer::Track::NoteOn(std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		if(tieMode) {
			//NoteOnTie(note, velocity);
		} else {
			NoteOnReal(note, velocity, length);
		}
		if(noteWait) {
			this->wait = length;
		}
	}

	void SequencePlayer::Track::NoteOnReal(std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		const InstrumentBank::LeafInstrument *noteInstrument = ResolveInstrumentForNote(note);
		auto voiceIndex = this->player->FindFreeVoice(noteInstrument->type);
		auto &voice = this->player->voices[voiceIndex];
		voice.StartNote(this, noteInstrument, note, length);
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

	void SequencePlayer::Track::SetInstrument(unsigned int instrumentId) {
		currentInstrument = player->bank->instruments.at(instrumentId).get();
	}

	std::uint8_t SequencePlayer::Track::readByteCommand() {
		std::uint8_t val = *nextCommand;
		++nextCommand;
		return val;
	}

	std::uint16_t SequencePlayer::Track::readShortCommand() {
		std::uint16_t val = *nextCommand;
		++nextCommand;
		val = val | (*nextCommand << 8);
		++nextCommand;

		return val;
	}

	unsigned int SequencePlayer::Track::readMidiVarLen() {
		unsigned int value = 0;

		for(;;) {

			std::uint8_t b = readByteCommand();

			value <<= 7;

			value |= (std::uint8_t)(b & 0x7F);

			if((b & 0x80) != 0x80) break;
		}

		return value;
	}

}