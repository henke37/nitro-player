#include "sequencePlayer.h"

#include <cassert>
#include <algorithm>

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
		this->amplitude = AMPLITUDE_THRESHOLD;

		ConfigureTimerRegister();
		ConfigureControlRegisters();
	}

	void SequencePlayer::Voice::Update() {
		if(state == SequencePlayer::VoiceState::Free) return;

		if(!(SCHANNEL_CR(voiceIndex) & SCHANNEL_ENABLE)) {
			Kill();
			return;
		}

		switch(state) {
		case SequencePlayer::VoiceState::Attacking:
		{
			int newAmpl = this->amplitude;
			int oldAmpl = this->amplitude >> 7;
			do
				newAmpl = (newAmpl * static_cast<int>(this->GetAttack())) / 256;
			while((newAmpl >> 7) == oldAmpl);
			this->amplitude = newAmpl;
			if(!this->amplitude)
				this->state = SequencePlayer::VoiceState::Decaying;
			break;
		}
		case SequencePlayer::VoiceState::Decaying:
		{
			this->amplitude -= static_cast<int>(this->GetDecay());
			int sustLvl = Cnv_Sust(this->GetSustain()) << 7;
			if(this->amplitude <= sustLvl) {
				this->amplitude = sustLvl;
				this->state = SequencePlayer::VoiceState::Sustaining;
			}
			break;
		}
		case SequencePlayer::VoiceState::Sustaining:
			//Nothing to do in this state
			break;
		case SequencePlayer::VoiceState::Releasing:
		{
			this->amplitude -= static_cast<int>(this->GetRelease());
			if(this->amplitude <= AMPLITUDE_THRESHOLD) {
				this->Kill();
				return;
			}
			break;
		}
		default:
			assert(0);
			break;
		}

		ConfigureVolumeRegister();
	}

	void SequencePlayer::Voice::Tick() {
		if(state == SequencePlayer::VoiceState::Free) return;

		if(state != VoiceState::Releasing) {
			if(!this->length) {
				Release();
			} else {
				--this->length;
			}
		}
	}

	void SequencePlayer::Voice::Release() {
		this->state = VoiceState::Releasing;
		this->length = 0;
	}

	void SequencePlayer::Voice::Kill() {
		this->state = VoiceState::Free;
		this->track = nullptr;
		this->currentInstrument = nullptr;
		this->length = 0;

		SCHANNEL_CR(voiceIndex) = SOUND_PAN(64);
	}

	void SequencePlayer::Voice::ConfigureControlRegisters() {
		std::uint32_t ctrVal = 0;
		ctrVal |= SOUND_PAN(ComputePan());

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->player->GetWave(pcmInstrument->archive, pcmInstrument->wave);

			assert(wave.waveData);

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

#define SOUND_VOLDIV(div) ((div) << 8)

	void SequencePlayer::Voice::ConfigureVolumeRegister() {
		int volume = ComputeVolume();

		std::uint32_t cr = SCHANNEL_CR(voiceIndex);
		cr &= ~(SOUND_VOL(0x7F) | SOUND_VOLDIV(3));

		cr |= SOUND_VOL(static_cast<int>(volumeTable[volume]));

		if(volume < AMPL_K - 240)
			cr |= SOUND_VOLDIV(3);
		else if(volume < AMPL_K - 120)
			cr |= SOUND_VOLDIV(2);
		else if(volume < AMPL_K - 60)
			cr |= SOUND_VOLDIV(1);

		SCHANNEL_CR(voiceIndex) = cr;
	}

	void SequencePlayer::Voice::ConfigureTimerRegister() {
		std::uint16_t timer;
		std::uint8_t baseNote;

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->player->GetWave(pcmInstrument->archive, pcmInstrument->wave);
			timer = wave.timerLen;
			baseNote = pcmInstrument->baseNote;
		} break;
		case InstrumentBank::InstrumentType::Pulse:
		case InstrumentBank::InstrumentType::Noise:
			timer = (std::uint16_t)SOUND_FREQ(440);
			baseNote = 69;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
		default:
			assert(0);
		}

		int adjustment = (this->note - baseNote) * 64;

		if(adjustment) timer = Timer_Adjust(timer, adjustment);

		SCHANNEL_TIMER(voiceIndex) = -timer;
	}

	int SequencePlayer::Voice::ComputeVolume() const {
		int volume = 0;// track->player->masterVol;
		//finalVol += track->player->sseqVol;
		volume += Cnv_Sust(track->volume);
		volume += Cnv_Sust(track->expression);
		if(volume < -AMPL_K)
			volume = -AMPL_K;

		volume += this->amplitude >> 7;
		volume += this->velocity;
		volume += AMPL_K;

		return std::clamp(volume, 0, AMPL_K);
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