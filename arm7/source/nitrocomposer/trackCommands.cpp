#include "sequencePlayer.h"

#include <cassert>
#include <cstdlib>

#include <nds/arm7/console.h>

#define NITROCOMPOSER_LOG_EFFECTS
#define NITROCOMPOSER_LOG_FLOW

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
			waitCounter = readMidiVarLen();
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
			sequence->StartTrack(trackId, offset);
#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Start #%d at 0x%x\n", GetId(), trackId, (unsigned)offset);
			consoleFlush();
#endif
		} break;

		case 0x94:
		{
			std::uint32_t offset = readTriByteCommand();
			SetNextCommand(offset);
#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Jump to 0x%x\n", GetId(), (unsigned)offset);
			consoleFlush();
#endif
		} break;

		case 0x95:
		{
			assert(stackPointer < 4);
			std::uint32_t offset = readTriByteCommand();

#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Call to 0x%x\n", GetId(), (unsigned)offset);
			consoleFlush();
#endif

			auto &stackRecord = stack[stackPointer];
			stackRecord.nextCommand = nextCommand;
			stackRecord.type = StackEntryType::Call;
			++stackPointer;

			SetNextCommand(offset);
		} break;

		case 0xA0:
		{
			ExecuteNextRandomCommand();
		} break;

		case 0xA1:
		{
			ExecuteNextVarCommand();
		} break;

		case 0xA2:
		{
			if(!lastComparisonResult) {
				std::uint8_t skippedCommand = readByteCommand();
				skipCommandArgs(skippedCommand);
			}
		} break;

		case 0xB0:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			sequence->SetVar(varId, val);
		} break;

		case 0xB1:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			sequence->SetVar(varId, sequence->GetVar(varId) + val);
		} break;

		case 0xB2:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			sequence->SetVar(varId, sequence->GetVar(varId) - val);
		} break;

		case 0xB3:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			sequence->SetVar(varId, sequence->GetVar(varId) * val);
		} break;

		case 0xB4:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			if(!val) break;
			sequence->SetVar(varId, sequence->GetVar(varId) / val);
		} break;

		case 0xB5:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			if(val < 0) {
				sequence->SetVar(varId, sequence->GetVar(varId) >> -val);
			} else {
				sequence->SetVar(varId, sequence->GetVar(varId) << val);
			}
		} break;

		case 0xB6:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			if(val < 0) {
				sequence->SetVar(varId, -(std::rand() % (-val + 1)));
			} else {
				sequence->SetVar(varId, std::rand() % (val + 1));
			}
		} break;

		case 0xB8:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			lastComparisonResult = sequence->GetVar(varId) == val;
		} break;

		case 0xB9:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			lastComparisonResult = sequence->GetVar(varId) >= val;
		} break;

		case 0xBA:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			lastComparisonResult = sequence->GetVar(varId) > val;
		} break;

		case 0xBB:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			lastComparisonResult = sequence->GetVar(varId) <= val;
		} break;

		case 0xBC:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			lastComparisonResult = sequence->GetVar(varId) < val;
		} break;

		case 0xBD:
		{
			std::uint8_t varId = readByteCommand();
			std::int16_t val = readShortCommand();
			assert(varId < numVariables);
			lastComparisonResult = sequence->GetVar(varId) != val;
		} break;

		case 0xC0:
		{
			pan = readByteCommand() - 64;
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Pan %d\n", pan);
			consoleFlush();
#endif
		} break;

		case 0xC1:
		{
			volume = readByteCommand();
		} break;

		case 0xC3:
		{
			transpose = readByteCommand();
		} break;

		case 0xC4:
		{
			pitchBend = readByteCommand();
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Pitch Bend %d\n", pitchBend);
			consoleFlush();
#endif
		} break;

		case 0xC5:
		{
			pitchBendRange = readByteCommand();
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
			ReleaseAllVoices();
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Tie Mode %s\n", tieMode ? "Y" : "N");
			consoleFlush();
#endif
		} break;

		case 0xC9:
		{
			std::uint8_t note = readByteCommand();
			lastPlayedNote = GetTransposedNote(note);
			portamento = true;
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Porta Start %u\n", note);
			consoleFlush();
#endif
		} break;

		case 0xCA:
		{
			modDepth = readByteCommand();
		} break;

		case 0xCB:
		{
			modSpeed = readByteCommand();
		} break;

		case 0xCC:
		{
			modMode = ModulationMode(readByteCommand());
#ifdef NITROCOMPOSER_LOG_EFFECTS
			switch(modMode) {
			case ModulationMode::Vibrato:
				consolePuts("Mod: Vibrato\n");
				break;
			case ModulationMode::Tremolo:
				consolePuts("Mod: Tremolo\n");
				break;
			case ModulationMode::Pan:
				consolePuts("Mod: Pan\n");
				break;
			}
			consoleFlush();
#endif
		} break;

		case 0xCD:
		{
			modRange = readByteCommand();
		} break;

		case 0xE0:
		{
			modDelay = readShortCommand();
		} break;

		case 0xCE:
		{
			portamento = readByteCommand() != 0;
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Porta %s\n", portamento?"Y":"N");
			consoleFlush();
#endif
		} break;

		case 0xCF:
		{
			portaTime = readByteCommand();
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
			assert(stackPointer < 4);
			std::uint8_t loopCount = readByteCommand();
			auto &stackRecord = stack[stackPointer];
			stackRecord.nextCommand = nextCommand;
			stackRecord.loopCounter = loopCount;
			stackRecord.type = StackEntryType::Loop;
			++stackPointer;

#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Loop start x%d\n", GetId(), loopCount);
			consoleFlush();
#endif
		}

		case 0xD5:
		{
			expression = readByteCommand();
		} break;

		case 0xD6:
		{
			std::uint8_t varId = readByteCommand();
			assert(varId < numVariables);
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Var %d = %d\n", varId, sequence->GetVar(varId));
			consoleFlush();
#endif
		} break;

		case 0xE1:
		{
			sequence->tempo = readShortCommand();
#ifdef NITROCOMPOSER_LOG_EFFECTS
			consolePrintf("Tempo set to %d\n", sequence->tempo);
			consoleFlush();
#endif
		} break;

		case 0xE3:
		{
			this->sweepPitch = readShortCommand();
		} break;

		case 0xFC:
		{
			assert(stackPointer > 0);
			--stackPointer;
			auto &stackRecord = stack[stackPointer];
			assert(stackRecord.type == StackEntryType::Loop);
			--stackRecord.loopCounter;
			if(stackRecord.loopCounter) {
				nextCommand = stackRecord.nextCommand;
				++stackPointer;
#ifdef NITROCOMPOSER_LOG_FLOW
				consolePrintf("#%d Loop repeat, %d remaining\n", GetId(), stackRecord.loopCounter);
				consoleFlush();
#endif
			} else {
#ifdef NITROCOMPOSER_LOG_FLOW
				consolePrintf("#%d Loop end\n", GetId());
				consoleFlush();
#endif
			}
		} break;

		case 0xFD:
		{
			assert(stackPointer > 0);
			--stackPointer;
			auto &stackRecord = stack[stackPointer];
			assert(stackRecord.type == StackEntryType::Call);
			nextCommand = stackRecord.nextCommand;
#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Return from Call\n", GetId());
			consoleFlush();
#endif
		} break;

		case 0xFE:
		{
			std::uint16_t tracks = readShortCommand();
#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Alloc %x\n", GetId(), tracks);
			consoleFlush();
#endif
		} break;

		case 0xFF: {
#ifdef NITROCOMPOSER_LOG_FLOW
			consolePrintf("#%d Fin.\n", GetId());
			consoleFlush();
#endif
			StopPlaying();
		} break;

		default:
			consolePrintf("#%d Skipping unknown command %x\n", GetId(), command);
			consoleFlush();
			skipCommandArgs(command);
			break;

		}
	}

	void SequencePlayer::Track::ExecuteNextRandomCommand() {
		std::uint8_t command = readByteCommand();
		switch(command) {
		default:
			consolePrintf("#%d Skipping unknown rnd command %x\n", GetId(), command);
			consoleFlush();
			skipCommandRandomArgs(command);
		}
	}

	void SequencePlayer::Track::ExecuteNextVarCommand() {
		std::uint8_t command = readByteCommand();
		switch(command) {
		default:
			consolePrintf("#%d Skipping unknown var command %x\n", GetId(), command);
			consoleFlush();
			skipCommandVarArgs(command);
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

	size_t SequencePlayer::Track::getCommandBytesLeft() const {
		return sequence->sequenceDataLength - (nextCommand - sequence->sequenceData);
	}

	std::uint8_t SequencePlayer::Track::readByteCommand() {
		assert(getCommandBytesLeft() >= 1);
		std::uint8_t val = *nextCommand;
		++nextCommand;
		return val;
	}

	std::uint16_t SequencePlayer::Track::readShortCommand() {
		assert(getCommandBytesLeft() >= 2);
		std::uint16_t val = *nextCommand;
		++nextCommand;
		val = val | (*nextCommand << 8);
		++nextCommand;

		return val;
	}

	std::uint32_t SequencePlayer::Track::readTriByteCommand() {
		assert(getCommandBytesLeft() >= 3);
		std::uint32_t val = *nextCommand;
		++nextCommand;
		val |= (*nextCommand << 8);
		++nextCommand;
		val |= (*nextCommand << 16);
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