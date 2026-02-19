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

	DebugFlags debugFlags {
		.logNotes = true,
		.logFlowControl = true,
		.logVarWrites = true,
		.logCommonEffects = false,
		.logUncommonEffects = true,
		.logBadData = true,
		.logNullNotes = true
	};

	void SequencePlayer::Init() {
		for(unsigned int var = 0; var < globalVariableCount; ++var) {
			globalVariables[var] = -1;
		}

		externalChannelReservations = 0;

		setupFifo();
	}

	void SequencePlayer::ReserveChannel(std::uint8_t hwChannel) {
		assert(hwChannel < voiceCount);
		assert(!(externalChannelReservations & BIT(hwChannel)));
		externalChannelReservations |= BIT(hwChannel);

		voices[hwChannel].Kill();
	}

	void SequencePlayer::UnreserveChannel(std::uint8_t hwChannel) {
		assert(hwChannel < voiceCount);
		assert(externalChannelReservations & BIT(hwChannel));
		externalChannelReservations &= ~BIT(hwChannel);
	}

	signed int SequencePlayer::FindFreeVoice(InstrumentBank::InstrumentType type, const Track *track) {
		size_t channelCount;
		const uint8_t *channelList;
		switch(type) {
		case InstrumentBank::InstrumentType::PCM:
		case InstrumentBank::InstrumentType::DirectPCM:
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
		case InstrumentBank::InstrumentType::Dummy:
		default:
			assert(0);
		}

		const PlayingSequence *sequence = track->GetSequence();

		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			auto &voice = voices[voiceIndex];
			if(voice.GetState() != VoiceState::Free) continue;
			if(!isVoiceAllowed(voiceIndex, sequence)) continue;
			return voiceIndex;
		}

		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			auto &voice = voices[voiceIndex];
			if(voice.GetState() != VoiceState::Releasing) continue;
			if(!isVoiceAllowed(voiceIndex, sequence)) continue;
			return voiceIndex;
		}

		int bestChannel = -1;
		for(unsigned int slotIndex = 0; slotIndex < channelCount; ++slotIndex) {
			auto voiceIndex = channelList[slotIndex];
			auto &voice = voices[voiceIndex];
			if(!isVoiceAllowed(voiceIndex, sequence)) continue;

			std::uint8_t voicePriority = voice.GetTrack()->GetPriority();

			if(bestChannel == -1) {
				if(voicePriority < track->GetPriority()) bestChannel = voiceIndex;
				continue;
			}

			auto &bestVoice = voices[bestChannel];

			std::uint8_t bestPriority = bestVoice.GetTrack()->GetPriority();

			if(voicePriority > bestPriority) continue;
			if(voicePriority < bestPriority) {
				bestChannel = voiceIndex;
				continue;
			}

			//equal priority, lowest volume loses
			int bestVolume = bestVoice.ComputeVolume();
			int voiceVolume = voice.ComputeVolume();

			//these values are pre transformed, so lower value means higher volume
			if(voiceVolume > bestVolume) {
				bestChannel = voiceIndex;
				continue;
			}
		}

		return bestChannel;
	}

	bool SequencePlayer::isVoiceAllowed(std::uint8_t voiceIndex, const PlayingSequence *sequence) const {
		if(sequence->allowedChannels && !(sequence->allowedChannels & BIT(voiceIndex))) return false;
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
		for(auto itr = playingSequences.begin(); itr != playingSequences.end(); ++itr) {
			auto &val = *itr;
			val->Update();
		}

		UpdateVoices();
	}

	void SequencePlayer::UpdateVoices() {
		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			voice.Update();
		}
	}

	SequencePlayer::PlayingSequence *SequencePlayer::GetPlayingSequence(std::int32_t playerId) {
		for(auto itr = playingSequences.begin(); itr != playingSequences.end(); ++itr) {
			auto &val = *itr;
			if(val->id == playerId) {
				return val.get();
			}
		}
		assert(0);
		return nullptr;
	}

	const SequencePlayer::PlayingSequence *SequencePlayer::GetPlayingSequence(std::int32_t playerId) const {
		for(auto itr = playingSequences.begin(); itr != playingSequences.end(); ++itr) {
			auto &val = *itr;
			if(val->id == playerId) {
				return val.get();
			}
		}
		assert(0);
		return nullptr;
	}

}