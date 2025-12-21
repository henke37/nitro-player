#include "sequencePlayer.h"

#include <cassert>
#include <algorithm>

#include <nds/bios.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/console.h>

//#define NITROCOMPOSER_LOG_VOICES

namespace NitroComposer {

	SequencePlayer::Voice::Voice(std::uint8_t voiceIndex) : voiceIndex(voiceIndex) {
	}

	void SequencePlayer::Voice::StartNote(Track *track, const InstrumentBank::LeafInstrument *instrument, std::uint8_t note, std::uint8_t velocity, unsigned int length) {
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

	void SequencePlayer::Voice::NextTieNote(std::uint8_t note, std::uint8_t velocity) {
		this->note = note;
		this->velocity = velocity;

		SetupPortamento();

		ConfigureTimerRegister();
		ConfigureControlRegisters();
	}

	void SequencePlayer::Voice::Update() {
		if(state == SequencePlayer::VoiceState::Free) return;

		assert(track);
		assert(currentInstrument);

		if(!IsHWChannelActive()) {
#ifdef NITROCOMPOSER_LOG_VOICES
			consolePuts("Voice fin");
			consoleFlush();
#endif
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
			if(this->length > 0) {
				--this->length;
				if(this->length == 0) Release();
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

		track->voiceCompleted(this);

		this->state = VoiceState::Free;
		this->track = nullptr;
		this->currentInstrument = nullptr;
		this->length = 0;

		REG_SOUNDXCNT(voiceIndex) = SOUNDXCNT_PAN(64);
	}

	void SequencePlayer::Voice::ConfigureControlRegisters() {
		std::uint32_t ctrVal = 0;
		ctrVal |= SOUNDXCNT_PAN(ComputePan());

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->sequence->GetWave(pcmInstrument->archive, pcmInstrument->wave);

			assert(wave.waveData);

			REG_SOUNDXSAD(voiceIndex) = reinterpret_cast<std::uintptr_t>(wave.waveData);

			ctrVal |= std::uint32_t(wave.encoding) << 29;
			ctrVal |= wave.loops ? SOUNDXCNT_REPEAT : SOUNDXCNT_ONE_SHOT;
			REG_SOUNDXLEN(voiceIndex) = wave.loopLength;
			REG_SOUNDXPNT(voiceIndex) = wave.loopStart;
		} break;
		case InstrumentBank::InstrumentType::Pulse:
		{
			auto pulseInstrument = static_cast<const InstrumentBank::PulseInstrument *>(currentInstrument);
			ctrVal |= pulseInstrument->duty << 24;
		}
		[[fallthrough]];
		case InstrumentBank::InstrumentType::Noise:
			ctrVal |= SOUNDXCNT_FORMAT_PSG;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
			assert(0);
		}

		ctrVal |= SOUNDXCNT_ENABLE;
		REG_SOUNDXCNT(voiceIndex) = ctrVal;
	}

	void SequencePlayer::Voice::ConfigureVolumeRegister() {
		int volume = ComputeVolume();

		std::uint32_t cr = REG_SOUNDXCNT(voiceIndex);
		cr &= ~(SOUNDXCNT_VOL_MUL(0x7F) | SOUNDXCNT_VOL_DIV(3));

		cr |= SOUNDXCNT_VOL_MUL(static_cast<int>(swiGetVolumeTable(volume)));

		if(volume < AMPL_K - 240)
			cr |= SOUNDXCNT_VOL_DIV(3);
		else if(volume < AMPL_K - 120)
			cr |= SOUNDXCNT_VOL_DIV(2);
		else if(volume < AMPL_K - 60)
			cr |= SOUNDXCNT_VOL_DIV(1);

		REG_SOUNDXCNT(voiceIndex) = cr;
	}

	void SequencePlayer::Voice::ConfigureTimerRegister() {
		std::uint16_t timerResetVal;

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave = track->sequence->GetWave(pcmInstrument->archive, pcmInstrument->wave);
			timerResetVal = wave.timerLen;
		} break;
		case InstrumentBank::InstrumentType::Pulse:
		case InstrumentBank::InstrumentType::Noise:
			timerResetVal = 8006;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
			assert(0);
			return;
		case InstrumentBank::InstrumentType::Null:
			assert(0);
			return;
		default:
			assert(0);
			return;
		}

		int adjustment = (this->note - currentInstrument->baseNote) * 64;

		adjustment += track->pitchBend * track->pitchBendRange >> 1;

		if(IsModulationActive(ModulationMode::Vibrato)) {
			adjustment += static_cast<int64_t>(GetModulationValue() * 60) >> 14;
		}
		if(IsPitchSweeping()) {
			adjustment += (static_cast<int64_t>(this->sweepPitch) * (this->sweepLength - this->sweepCounter)) / sweepLength;
		}

		if(adjustment) timerResetVal = Timer_Adjust(timerResetVal, adjustment);

		if(currentInstrument->type == InstrumentBank::InstrumentType::Pulse) {
			timerResetVal &= 0xFFFC;
		}

		REG_SOUNDXTMR(voiceIndex) = -timerResetVal;
	}

	bool SequencePlayer::Voice::IsHWChannelActive() const {
		return (REG_SOUNDXCNT(voiceIndex) & SOUNDXCNT_ENABLE) == SOUNDXCNT_ENABLE;
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