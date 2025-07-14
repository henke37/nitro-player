#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm7/console.h>

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
			assert(stackPointer < 4);
			std::uint32_t offset = readTriByteCommand();

			auto &stackRecord = stack[stackPointer];
			stackRecord.nextCommand = nextCommand;
			stackRecord.type = StackEntryType::Call;
			++stackPointer;

			SetNextCommand(offset);
		} break;

		case 0xA2:
		{
			if(!lastComparisonResult) {
				std::uint8_t skippedCommand = readByteCommand();
				skipCommandArgs(skippedCommand);
			}
		} break;

		case 0xC0:
		{
			pan = readByteCommand();
		} break;

		case 0xC1:
		{
			volume = readByteCommand();
		} break;

		case 0xC6:
		{
			priority = readByteCommand();
		} break;

		case 0xC7:
		{
			noteWait = readByteCommand() != 0;
		} break;

		case 0xC8:
		{
			tieMode = readByteCommand() != 0;
		} break;

		case 0xD0:
		{
			attack = readByteCommand();
		} break;

		case 0xD1:
		{
			decay = readByteCommand();
		} break;

		case 0xD2:
		{
			sustain = readByteCommand();
		} break;

		case 0xD3:
		{
			release = readByteCommand();
		} break;

		case 0xD4:
		{
			auto &stackRecord = stack[stackPointer];
			stackRecord.nextCommand = nextCommand;
			stackRecord.loopCounter = readByteCommand();
			stackRecord.type = StackEntryType::Loop;
			++stackPointer;
		}

		case 0xD5:
		{
			expression = readByteCommand();
		} break;


		case 0xE1:
		{
			player->tempo = readShortCommand();
		} break;

		case 0xFC:
		{
			assert(stackPointer > 0);
			--stackPointer;
			auto &stackRecord = stack[stackPointer];
			assert(stackRecord.type == StackEntryType::Loop);
			--stackRecord.loopCounter;
			if(stackRecord.loopCounter) ++stackPointer;
		} break;

		case 0xFD:
		{
			assert(stackPointer > 0);
			--stackPointer;
			auto &stackRecord = stack[stackPointer];
			assert(stackRecord.type == StackEntryType::Call);
			nextCommand = stackRecord.nextCommand;
		} break;

		case 0xFE:
		{
			nextCommand += 2;
		}

		case 0xFF: {
			StopPlaying();
		} break;

		default:
			consolePrintf("Skipping unknown command %x\n", command);
			consoleFlush();
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
		case 0xA0:
			skipCommandRandomArgs(readByteCommand());
			break;
		case 0xA1:
			skipCommandVarArgs(readByteCommand());
			break;
		case 0xA2:
			skipCommandArgs(readByteCommand());
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

	void SequencePlayer::Track::skipCommandRandomArgs(std::uint8_t command) {
		if(command < 0x80) {
			nextCommand += 5;
			return;
		}
		switch(command) {
		case 0x80:
		case 0x81:
			nextCommand += 4;
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
			nextCommand += 5;
			break;
		case 0xC0:
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xE0:
		case 0xCF:
		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD5:
		case 0xE3:
			nextCommand += 4;
			break;
		default:
			assert(0);
		}
	}

	void SequencePlayer::Track::skipCommandVarArgs(std::uint8_t command) {
		if(command < 0x80) {
			nextCommand += 2;
			return;
		}

		switch(command) {
		case 0x80:
		case 0x81:
			nextCommand += 1;
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
			nextCommand += 2;
			break;
		case 0xC0:
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xE0:
		case 0xCF:
		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xE3:
			nextCommand += 1;
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