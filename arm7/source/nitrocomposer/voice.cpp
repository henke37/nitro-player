#include "sequencePlayer.h"

#include <cassert>
#include <algorithm>

#include <nds/arm7/audio.h>
#include <nds/arm7/console.h>

namespace NitroComposer {

	SequencePlayer::Voice::Voice(std::uint8_t voiceIndex) : voiceIndex(voiceIndex) {
	}

	void SequencePlayer::Voice::StartNote(const Track *track, const InstrumentBank::LeafInstrument *instrument, std::uint8_t note, std::uint8_t velocity, unsigned int length) {
		assert(track);
		assert(instrument);
		assert(instrument->type != InstrumentBank::InstrumentType::Null);
		assert(instrument->type != InstrumentBank::InstrumentType::Drumkit);
		assert(instrument->type != InstrumentBank::InstrumentType::Split);

		this->currentInstrument = instrument;
		this->track = track;

		this->note = note;
		this->velocity = velocity;
		this->length = length;

		this->state = VoiceState::Attacking;
		this->amplitude = AMPLITUDE_THRESHOLD;

		this->modCounter = 0;
		this->modDelayCounter = 0;

		SetupPortamento();

		ConfigureTimerRegister();
		ConfigureControlRegisters();
	}

	void SequencePlayer::Voice::Update() {
		if(state == SequencePlayer::VoiceState::Free) return;

		assert(track);
		assert(currentInstrument);

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

		UpdateModulation();
		UpdatePitchSweep();

		ConfigureTimerRegister();
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
		if(this->state == VoiceState::Free || this->state == VoiceState::Releasing) return;
		this->state = VoiceState::Releasing;
		this->length = 0;
	}

	void SequencePlayer::Voice::Kill() {
		if(this->state == VoiceState::Free) return;
		this->track->sequence->voices[this->voiceIndex] = nullptr;

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
			auto &wave = track->sequence->GetWave(pcmInstrument->archive, pcmInstrument->wave);

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
		std::uint16_t timerResetVal;
		std::uint8_t baseNote;

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->sequence->GetWave(pcmInstrument->archive, pcmInstrument->wave);
			timerResetVal = wave.timerLen;
			baseNote = pcmInstrument->baseNote;
		} break;
		case InstrumentBank::InstrumentType::Pulse:
		case InstrumentBank::InstrumentType::Noise:
			timerResetVal = (std::uint16_t)SOUND_FREQ(440);
			baseNote = 69;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
		default:
			assert(0);
		}

		int adjustment = (this->note - baseNote) * 64;

		adjustment += track->pitchBend * track->pitchBendRange >> 1;

		if(IsModulationActive(ModulationMode::Vibrato)) {
			adjustment += static_cast<int64_t>(GetModulationValue() * 60) >> 14;
		}
		if(IsPitchSweeping()) {
			adjustment += (static_cast<int64_t>(this->sweepPitch) * (this->sweepLength - this->sweepCounter)) / sweepLength;
		}

		if(adjustment) timerResetVal = Timer_Adjust(timerResetVal, adjustment);

		SCHANNEL_TIMER(voiceIndex) = -timerResetVal;
	}

	int SequencePlayer::Voice::ComputeVolume() const {
		int volume = 0;// track->player->masterVol;
		volume += track->sequence->sequenceVolume;
		volume += Cnv_Sust(track->volume);
		volume += Cnv_Sust(track->expression);
		if(volume < -AMPL_K)
			volume = -AMPL_K;

		if(IsModulationActive(ModulationMode::Tremolo)) {
			volume += GetModulationValue() >> 8;
		}

		volume += this->amplitude >> 7;
		volume += Cnv_Sust(this->velocity);
		volume += AMPL_K;

		return std::clamp(volume, 0, AMPL_K);
	}

	std::uint8_t SequencePlayer::Voice::ComputePan() const {
		int pan = currentInstrument->pan - 64;
		pan += this->track->pan;
		pan += 64;

		if(IsModulationActive(ModulationMode::Pan)) {
			pan += GetModulationValue() >> 8;
		}

		return std::clamp(pan, 0, 127);
	}

	void SequencePlayer::Voice::SetupPortamento() {
		this->sweepCounter = 0;
		if(!track->portamento) {
			this->sweepLength = 0;
			return;
		}
		bool manualSweep = track->portaTime == 0;
	
		int diff = (static_cast<int>(track->lastPlayedNote) - static_cast<int>(this->note)) << 22;

		this->sweepPitch = track->sweepPitch + (diff >> 16);

		if(manualSweep) {
			this->sweepLength = this->length;
		} else {
			int sq_time = static_cast<uint32_t>(track->portaTime) * static_cast<uint32_t>(track->portaTime);
			int abs_sp = std::abs(this->sweepPitch);
			this->sweepLength = (abs_sp * sq_time) >> 11;
		}
	}

	void SequencePlayer::Voice::UpdatePitchSweep() {
		if(this->sweepCounter < this->sweepLength) {
			++this->sweepCounter;
		}
	}

	bool SequencePlayer::Voice::IsPitchSweeping() const {
		return this->sweepPitch && this->sweepLength && this->sweepCounter <= this->sweepLength;
	}

	void SequencePlayer::Voice::UpdateModulation() {
		if(track->modDepth == 0) return;
		if(this->modDelayCounter < track->modDelay) {
			++this->modDelayCounter;
			return;
		}
		uint16_t speed = static_cast<uint16_t>(track->modSpeed) << 6;
		this->modCounter = (this->modCounter + speed) & 0x7FFF;
	}

	bool SequencePlayer::Voice::IsModulationActive(ModulationMode mode) const {
		if(track->modDepth == 0) return false;
		if(this->modDelayCounter < track->modDelay) return false;
		return mode==track->modMode;
	}

	int SequencePlayer::Voice::GetModulationValue() const {
		return Cnv_Sine(this->modCounter >> 8) * track->modRange * track->modDepth;
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