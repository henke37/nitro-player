#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm7/audio.h>

namespace NitroComposer {

	void SequencePlayer::Voice::Init(unsigned int voiceIndex, SequencePlayer *player) {
		this->voiceIndex = voiceIndex;
		this->player = player;
	}

	void SequencePlayer::Voice::Tick() {}

	void SequencePlayer::Voice::ConfigureControlRegisters() {
		std::uint32_t ctrVal = ComputeVolume();
		ctrVal |= SOUND_PAN(ComputePan());

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto pcmInstrument = static_cast<const InstrumentBank::PCMInstrument *>(currentInstrument);
			auto &wave=player->GetWave(pcmInstrument->archive, pcmInstrument->wave);
			SCHANNEL_SOURCE(voiceIndex) = reinterpret_cast<std::uintptr_t>(wave.waveData);

			ctrVal |= std::uint32_t(wave.encoding) << 29;
			ctrVal |= wave.loops ? SOUND_REPEAT : SOUND_ONE_SHOT;
			SCHANNEL_LENGTH(voiceIndex) = wave.loopLength;
			SCHANNEL_REPEAT_POINT(voiceIndex) = wave.loopStart;
		}
		case InstrumentBank::InstrumentType::Pulse:
		{
			auto pulseInstrument = static_cast<const InstrumentBank::PulseInstrument *>(currentInstrument);
			ctrVal |= pulseInstrument->duty << 24;
		}
			//fall thru
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

	std::uint8_t SequencePlayer::Voice::ComputeVolume() const {
		return 64;
	}

}