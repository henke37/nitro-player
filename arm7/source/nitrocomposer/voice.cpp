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
		ctrVal |= ComputePan() << 16;

		switch(currentInstrument->type) {
		case InstrumentBank::InstrumentType::PCM:
		{
			auto *pcmInstrument = static_cast<InstrumentBank::PCMInstrument *>(currentInstrument.get());
			auto &wave=player->GetWave(pcmInstrument->archive, pcmInstrument->wave);
			SCHANNEL_SOURCE(voiceIndex) = reinterpret_cast<std::uintptr_t>(wave.waveData);

			ctrVal |= std::uint32_t(wave.encoding) << 29;
		}
		case InstrumentBank::InstrumentType::Pulse:
		{
			auto *pulseInstrument = static_cast<InstrumentBank::PulseInstrument *>(currentInstrument.get());
			ctrVal |= pulseInstrument->duty << 29;
		}
			//fall thru
		case InstrumentBank::InstrumentType::Noise:
			ctrVal |= 3 << 29;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
			assert(0);
		}

		ctrVal |= 1 << 31;
		SCHANNEL_CR(voiceIndex) = ctrVal;
	}

	void SequencePlayer::Voice::ConfigureVolumeRegister() {
		SCHANNEL_VOL(voiceIndex) = ComputeVolume();
	}

	std::uint8_t SequencePlayer::Voice::ComputeVolume() const {
		return 64;
	}

}