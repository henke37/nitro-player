#include "sequencePlayer.h"
#include "streamPlayer.h"

#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/arm7/console.h>
#include <nds/system.h>
#include <nds/arm7/audio.h>

#include "nitroComposer/ipc.h"

#include <cassert>
#include <algorithm>

namespace NitroComposer {

	void SequencePlayer::setupFifo() {
		fifoSetDatamsgHandler(FIFO_NITRO_COMPOSER, fifoDatagramHandler, nullptr);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes, [[maybe_unused]] void *userdata) {
		sequencePlayer.fifoDatagramHandler(num_bytes);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes) {
		u8 fifoBuffer[fifoBuffSize];

		assert(num_bytes <= (int)fifoBuffSize);

		int written = fifoGetDatamsg(FIFO_NITRO_COMPOSER, num_bytes, fifoBuffer);
		assert(written > 0);

		auto ipc = reinterpret_cast<BaseIPC *>(fifoBuffer);

		switch(ipc->command) {

		case BaseIPC::CommandType::PowerOn:
		{
			enableSound();

			setupTimer();
		} break;

		case BaseIPC::CommandType::AllocSequencePlayer:
		{
			SequencePlayerIPC *allocIpc = static_cast<SequencePlayerIPC *>(ipc);
			playingSequences.emplace_back(std::make_unique<PlayingSequence>(allocIpc->playerId));
			bool success = fifoSendValue32(FIFO_NITRO_COMPOSER, allocIpc->playerId);
			assert(success);
		} break;
		case BaseIPC::CommandType::DeallocSequencePlayer:
		{
			SequencePlayerIPC *allocIpc = static_cast<SequencePlayerIPC *>(ipc);

			auto it = std::find_if(playingSequences.begin(), playingSequences.end(),
				[allocIpc](const std::unique_ptr<PlayingSequence> &seq) {
					return seq->id == allocIpc->playerId;
				});
			playingSequences.erase(it);
			
			bool success = fifoSendValue32(FIFO_NITRO_COMPOSER, allocIpc->playerId);
			assert(success);
		} break;

		case BaseIPC::CommandType::SetVar:
		{
			SetVarIPC *setVarIpc = static_cast<SetVarIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(setVarIpc->playerId);
			sequence->SetVar(setVarIpc->var, setVarIpc->val);
		} break;

		case BaseIPC::CommandType::GetVar:
		{
			GetVarIPC *getVarIpc = static_cast<GetVarIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(getVarIpc->playerId);
			std::int16_t val = sequence->GetVar(getVarIpc->var);
			bool success = fifoSendValue32(FIFO_NITRO_COMPOSER, val);
			assert(success);
		} break;

		case BaseIPC::CommandType::SetTempo:
		{
			SetTempoIPC *setTempoIpc = static_cast<SetTempoIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(setTempoIpc->playerId);
			sequence->tempo = setTempoIpc->tempo;
		} break;
		case BaseIPC::CommandType::SetMainVolume:
		{
			SetMainVolumeIPC *setMainVolumeIpc = static_cast<SetMainVolumeIPC *>(ipc);
			this->mainVolume = setMainVolumeIpc->volume;
			REG_MASTER_VOLUME = setMainVolumeIpc->volume;
		} break;

		case BaseIPC::CommandType::LoadBank:
		{
			LoadBankIPC *loadBankIpc = static_cast<LoadBankIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(loadBankIpc->playerId);
			sequence->AbortSequence(true);
			sequence->bank = loadBankIpc->bank;

			consolePrintf("Loaded %d instruments\n", sequence->bank->instruments.size());
			consoleFlush();
		} break;

		case BaseIPC::CommandType::LoadWaveArchive:
		{
			LoadWaveArchiveIPC *loadWaveArchiveIpc = static_cast<LoadWaveArchiveIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(loadWaveArchiveIpc->playerId);
			assert(loadWaveArchiveIpc->slot < PlayingSequence::numWaveArchs);
			sequence->AbortSequence(true);
			sequence->waveArchs[loadWaveArchiveIpc->slot] = loadWaveArchiveIpc->archive;

			if(!loadWaveArchiveIpc->archive) break;

			consolePrintf("Loaded bank %d with %d waves\n", loadWaveArchiveIpc->slot, loadWaveArchiveIpc->archive->waves.size());
			consoleFlush();
		} break;

		case BaseIPC::CommandType::PlaySequence:
		{
			PlayTrackIPC *playTrackIpc = static_cast<PlayTrackIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(playTrackIpc->playerId);
			sequence->Reset();
			sequence->allowedChannels = playTrackIpc->channelMask;
			sequence->sequenceVolume = playTrackIpc->sequenceVolume;
			sequence->priority = playTrackIpc->sequencePriority;
			sequence->PlaySequence(playTrackIpc->sequenceData, playTrackIpc->length, playTrackIpc->startPos);
		} break;

		case BaseIPC::CommandType::StopSequence:
		{
			SequencePlayerIPC *sequenceIpc = static_cast<SequencePlayerIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(sequenceIpc->playerId);
			sequence->AbortSequence(false);
			fifoSendValue32(FIFO_NITRO_COMPOSER, 41);
		} break;

		case BaseIPC::CommandType::KillSequence:
		{
			SequencePlayerIPC *sequenceIpc = static_cast<SequencePlayerIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(sequenceIpc->playerId);
			sequence->AbortSequence(true);
			fifoSendValue32(FIFO_NITRO_COMPOSER, 43);
		} break;

		case BaseIPC::CommandType::MuteTrack:
		{
			MuteTrackIPC *muteTrackIpc = static_cast<MuteTrackIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(muteTrackIpc->playerId);
			sequence->SetTrackMute(muteTrackIpc->trackId, muteTrackIpc->mute ? Track::MuteMode::MuteAndRelease : Track::MuteMode::Clear);
		} break;

		case BaseIPC::CommandType::SetAllowedChannels:
		{
			SetAllowedChannelsIPC *allowedChannelsIpc = static_cast<SetAllowedChannelsIPC *>(ipc);
			PlayingSequence *sequence = GetPlayingSequence(allowedChannelsIpc->playerId);
			sequence->allowedChannels = allowedChannelsIpc->channels;
		} break;

		case BaseIPC::CommandType::AllocStreamPlayer:
		{
			assert(!streamPlayer);
			StreamPlayerAllocIPC *allocIpc = static_cast<StreamPlayerAllocIPC *>(ipc);
			assert(allocIpc->channelCount == 1 || allocIpc->channelCount == 2);
			if(allocIpc->channelCount == 1) {
				streamPlayer = std::make_unique<StreamPlayer>(allocIpc->playbackBuffSize, allocIpc->timerId, allocIpc->hwChannels[0]);
			} else {
				streamPlayer = std::make_unique<StreamPlayer>(allocIpc->playbackBuffSize, allocIpc->timerId, allocIpc->hwChannels[0], allocIpc->hwChannels[1]);
			}
		} break;

		case BaseIPC::CommandType::DeallocStreamPlayer:
		{
			assert(streamPlayer);
			streamPlayer.reset();
		} break;

		case BaseIPC::CommandType::InitStream:
		{
			assert(streamPlayer);
			auto initStreamIpc = static_cast<InitStreamIPC *>(ipc);
			streamPlayer->Init(initStreamIpc->encoding, initStreamIpc->stereo, initStreamIpc->sampleRate);
		} break;

		case BaseIPC::CommandType::StopStream:
		{
			assert(streamPlayer);
			streamPlayer->Stop(false);
		} break;

		case BaseIPC::CommandType::StopStreamInstantly:
		{
			assert(streamPlayer);
			streamPlayer->Stop(true);
		} break;

		case BaseIPC::CommandType::StreamSetVolume:
		{
			assert(streamPlayer);
			auto setVolumeIpc = static_cast<StreamVolumeIPC *>(ipc);
			streamPlayer->SetVolume(setVolumeIpc->volume);
		} break;

		case BaseIPC::CommandType::StreamSetPan:
		{
			assert(streamPlayer);
			auto setPanIpc = static_cast<StreamPanIPC *>(ipc);
			streamPlayer->SetPan(setPanIpc->pan);
		} break;

		case BaseIPC::CommandType::StreamPushBlock:
		{
			assert(streamPlayer);
			auto pushBlockIpc = static_cast<StreamPushBlockIPC *>(ipc);

			std::unique_ptr<StreamBlock> block=std::make_unique<StreamBlock>();
			block->blockId = pushBlockIpc->blockId;
			block->blockDataSize = pushBlockIpc->blockDataSize;
			block->blockSampleCount = pushBlockIpc->blockSampleCount;
			block->startPos = pushBlockIpc->startPos;
			block->blockData[0] = pushBlockIpc->blockData[0];
			block->blockData[1] = pushBlockIpc->blockData[1];

			streamPlayer->AddBlock(std::move(block));
		} break;

		default:
			assert(0);
		}
	}
	void SequencePlayer::sendFifoSequenceStatus([[maybe_unused]] const PlayingSequence &sequence) {
		bool success;
		SequenceStatusEventIPC statusIpc;
		statusIpc.eventId = AsyncEventIPC::EventType::SequenceEnded;
		statusIpc.playerId = 1;//TODO: multiple players
		success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequenceStatusEventIPC), (u8 *)&statusIpc);
		assert(success);
		success = fifoSendAddress(FIFO_NITRO_COMPOSER, (void*)0x020C0DE0);//kludge for fifo system
		assert(success);
	}
	void StreamPlayer::sendFifoStreamRetireBlock(std::uint32_t blockId) {
		bool success;
		StreamRetireBlockIPC ipc;
		ipc.eventId = AsyncEventIPC::EventType::StreamRetireBlock;
		ipc.blockId = blockId;
		success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(StreamRetireBlockIPC), (u8 *)&ipc);
		assert(success);
		success = fifoSendAddress(FIFO_NITRO_COMPOSER, (void*)0x020C0DE0);//kludge for fifo system
		assert(success);
	}
	void StreamPlayer::sendFifoStreamOutOfData() {
		bool success;
		AsyncEventIPC ipc;
		ipc.eventId = AsyncEventIPC::EventType::StreamOutOfData;
		success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(AsyncEventIPC), (u8 *)&ipc);
		assert(success);
		success = fifoSendAddress(FIFO_NITRO_COMPOSER, (void*)0x020C0DE0);//kludge for fifo system
		assert(success);
	}
	void StreamPlayer::sendFifoStreamPlaybackEnded() {
		bool success;
		AsyncEventIPC ipc;
		ipc.eventId = AsyncEventIPC::EventType::StreamEnded;
		success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(AsyncEventIPC), (u8 *)&ipc);
		assert(success);
		success = fifoSendAddress(FIFO_NITRO_COMPOSER, (void *)0x020C0DE0);//kludge for fifo system
		assert(success);
	}
}