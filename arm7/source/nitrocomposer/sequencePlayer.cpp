#include "sequencePlayer.h"

#include <nds/timers.h>
#include <nds/interrupts.h>
#include <nds/arm7/console.h>
#include <nds/system.h>
#include <nds/arm7/audio.h>

#include <cassert>

namespace NitroComposer {

	static const uint8_t pcmChannels[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const uint8_t psgChannels[] = { 8, 9, 10, 11, 12, 13 };
	static const uint8_t noiseChannels[] = { 14, 15 };

	SequencePlayer sequencePlayer;

	void SequencePlayer::Init() {
		for(unsigned int var = 0; var < globalVariableCount; ++var) {
			globalVariables[var] = -1;
		}

		playingSequence.Init();

		externalChannelReservations = 0;
		allowedChannels = 0xFF;

		setupFifo();
	}


	signed int SequencePlayer::FindFreeVoice(InstrumentBank::InstrumentType type, const PlayingSequence *) {
		size_t channelCount;
		const uint8_t *channelList;
		switch(type) {
		case InstrumentBank::InstrumentType::PCM:
			channelCount = 16;
			channelList = pcmChannels;
			break;
		case InstrumentBank::InstrumentType::Pulse:
			channelCount = 6;
			channelList = psgChannels;
			break;
		case InstrumentBank::InstrumentType::Noise:
			channelCount = 2;
			channelList = noiseChannels;
			break;
		case InstrumentBank::InstrumentType::Drumkit:
		case InstrumentBank::InstrumentType::Split:
		case InstrumentBank::InstrumentType::Null:
		default:
			assert(0);
		}

		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			if(!isVoiceAllowed(voiceIndex)) continue;
			auto &voice = voices[voiceIndex];
			if(voice.state == VoiceState::Free) return voiceIndex;
		}

		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			if(!isVoiceAllowed(voiceIndex)) continue;
			auto &voice = voices[voiceIndex];
			if(voice.state == VoiceState::Releasing) return voiceIndex;
		}

		//TODO: voice stealing

		return -1;
	}

	bool SequencePlayer::isVoiceAllowed(std::uint8_t voiceIndex) const {
		if(!(allowedChannels & BIT(voiceIndex)) && allowedChannels) return false;
		if((externalChannelReservations & BIT(voiceIndex)) && externalChannelReservations) return false;

		return true;
	}

	void SequencePlayer::setupTimer() {
		timerStart(LIBNDS_DEFAULT_TIMER_MUSIC, ClockDivider_64, -2728, ISR);
	}

	void SequencePlayer::ISR() {
		sequencePlayer.Update();
	}

	void SequencePlayer::Update() {
		playingSequence.Update();
		UpdateVoices();
	}

	void SequencePlayer::UpdateVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			voice.Update();
		}
	}

}