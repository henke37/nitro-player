#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm7/audio.h>

namespace NitroComposer {

	void SequencePlayer::Voice::Init(unsigned int voiceIndex) {
		this->voiceIndex = voiceIndex;
	}

	void SequencePlayer::Voice::Tick() {}

	void SequencePlayer::Voice::ConfigureControlRegisters() {
	
	}

	void SequencePlayer::Voice::ConfigureVolumeRegister() {
		std::uint8_t loudness = 127;
		SCHANNEL_VOL(voiceIndex) = loudness;
	}

}