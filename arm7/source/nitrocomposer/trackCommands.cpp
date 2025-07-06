#include "sequencePlayer.h"

#include <cassert>

namespace NitroComposer {

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

		case 0x93:
		{
			std::uint8_t trackId = readByteCommand();
			std::uint32_t offset = readTriByteCommand();
			player->StartTrack(trackId, offset);
		} break;

		case 0x94:
		{
			std::uint32_t offset = readTriByteCommand();
			SetNextCommand(offset);
		} break;

		case 0x95:
		{
			//TODO: push callstack
			std::uint32_t offset = readTriByteCommand();
			SetNextCommand(offset);
		} break;

		case 0xC7:
		{
			noteWait = readByteCommand() != 0;
		} break;

		case 0xC8:
		{
			tieMode = readByteCommand() != 0;
		} break;

		case 0xE1:
		{
			player->tempo = readShortCommand();
		} break;

		case 0xFD:
		{
			//TODO: pop callstack
		} break;

		case 0xFE:
		{
			nextCommand += 2;
		}

		case 0xFF: {
			StopPlaying();
		} break;

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

	std::uint32_t SequencePlayer::Track::readTriByteCommand() {
		std::uint32_t val = *nextCommand;
		++nextCommand;
		val = val | (*nextCommand << 8);
		++nextCommand;
		val = val | (*nextCommand << 16);
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