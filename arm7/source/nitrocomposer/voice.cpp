#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm7/audio.h>
#include <nds/arm7/console.h>

namespace NitroComposer {

	void SequencePlayer::Voice::Init(unsigned int voiceIndex) {
		this->voiceIndex = voiceIndex;
	}

	void SequencePlayer::Voice::StartNote(const Track *track, const InstrumentBank::LeafInstrument *instrument, std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		this->currentInstrument = instrument;
		this->track = track;

		this->note = note;
		this->velocity = velocity;
		this->length = length;

		this->state = VoiceState::Attacking;

		ConfigureTimerRegister();
		ConfigureControlRegisters();
	}

	void SequencePlayer::Voice::Tick() {}

	void SequencePlayer::Voice::ConfigureControlRegisters() {
		std::uint32_t ctrVal = SOUND_VOL(ComputeVolume());
		ctrVal |= SOUND_PAN(ComputePan());

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->player->GetWave(pcmInstrument->archive, pcmInstrument->wave);

			assert(wave.waveData);

			consolePrintf(" %d/%d\n", wave.loopLength, wave.loopStart);
			consoleFlush();

			SCHANNEL_SOURCE(voiceIndex) = reinterpret_cast<std::uintptr_t>(wave.waveData);

			ctrVal |= std::uint32_t(wave.encoding) << 29;
			ctrVal |= wave.loops ? SOUND_REPEAT : SOUND_ONE_SHOT;
			SCHANNEL_LENGTH(voiceIndex) = wave.loopLength;
			SCHANNEL_REPEAT_POINT(voiceIndex) = wave.loopStart;
		} break;
		case InstrumentBank::InstrumentType::Pulse:
		{
			auto pulseInstrument = static_cast<const InstrumentBank::PulseInstrument *>(currentInstrument);
			ctrVal |= pulseInstrument->duty << 24;
		}
		[[fallthrough]];
		case InstrumentBank::InstrumentType::Noise:
			ctrVal |= SOUND_FORMAT_PSG;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
			assert(0);
		}

		ctrVal |= SCHANNEL_ENABLE;
		SCHANNEL_CR(voiceIndex) = ctrVal;
	}

	void SequencePlayer::Voice::ConfigureVolumeRegister() {
		SCHANNEL_VOL(voiceIndex) = ComputeVolume();
	}

	void SequencePlayer::Voice::ConfigureTimerRegister() {
		std::uint16_t baseTimer;
		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->player->GetWave(pcmInstrument->archive, pcmInstrument->wave);
			baseTimer = wave.timerLen;
		} break;
		case InstrumentBank::InstrumentType::Pulse:
		case InstrumentBank::InstrumentType::Noise:
			baseTimer = (std::uint16_t)SOUND_FREQ(440);
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
		default:
			assert(0);
		}

		SCHANNEL_TIMER(voiceIndex) = -baseTimer;
	}

	std::uint8_t SequencePlayer::Voice::ComputeVolume() const {
		return 127;
	}

	std::uint8_t SequencePlayer::Voice::ComputePan() const {
		return currentInstrument->pan;
	}

	std::uint8_t SequencePlayer::Voice::GetAttack() const {
		return track->attack != 0xFF ? track->attack : currentInstrument->attack;
	}

	std::uint8_t SequencePlayer::Voice::GetDecay() const {
		return track->decay != 0xFF ? track->decay : currentInstrument->decay;
	}

	std::uint8_t SequencePlayer::Voice::GetSustain() const {
		return track->sustain != 0xFF ? track->sustain : currentInstrument->sustain;
	}

	std::uint8_t SequencePlayer::Voice::GetRelease() const {
		return track->release != 0xFF ? track->release : currentInstrument->release;
	}

}