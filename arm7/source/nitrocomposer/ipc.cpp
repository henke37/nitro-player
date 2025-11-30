#include "sequencePlayer.h"
#include "streamPlayer.h"

#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/arm7/console.h>
#include <nds/system.h>
#include <nds/arm7/audio.h>

#include "nitroComposer/ipc.h"

#include <cassert>

namespace NitroComposer {

	void SequencePlayer::setupFifo() {
		fifoSetDatamsgHandler(FIFO_NITRO_COMPOSER, fifoDatagramHandler, nullptr);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes, void *userdata) {
		sequencePlayer.fifoDatagramHandler(num_bytes);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes) {
		u8 fifoBuffer[fifoBuffSize];
		fifoGetDatamsg(FIFO_NITRO_COMPOSER, fifoBuffSize, fifoBuffer);

		auto ipc = reinterpret_cast<BaseIPC *>(fifoBuffer);

		switch(ipc->command) {

		case BaseIPC::CommandType::PowerOn:
		{
			enableSound();

			setupTimer();
		} break;

		case BaseIPC::CommandType::SetVar:
		{
			SetVarIPC *setVarIpc = static_cast<SetVarIPC *>(ipc);
			playingSequence.SetVar(setVarIpc->var, setVarIpc->val);
		} break;

		case BaseIPC::CommandType::GetVar:
		{
			GetVarIPC *getVarIpc = static_cast<GetVarIPC *>(ipc);
			std::int16_t val = playingSequence.GetVar(getVarIpc->var);
			bool success = fifoSendValue32(FIFO_NITRO_COMPOSER, val);
			assert(success);
		} break;

		case BaseIPC::CommandType::SetTempo:
		{
			SetTempoIPC *setTempoIpc = static_cast<SetTempoIPC *>(ipc);
			this->playingSequence.tempo = setTempoIpc->tempo;
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
			playingSequence.AbortSequence(true);
			this->playingSequence.bank = loadBankIpc->bank;

			consolePrintf("Loaded %d instruments\n", this->playingSequence.bank->instruments.size());
			consoleFlush();
		} break;

		case BaseIPC::CommandType::LoadWaveArchive:
		{
			LoadWaveArchiveIPC *loadWaveArchiveIpc = static_cast<LoadWaveArchiveIPC *>(ipc);
			assert(loadWaveArchiveIpc->slot < PlayingSequence::numWaveArchs);
			playingSequence.AbortSequence(true);
			this->playingSequence.waveArchs[loadWaveArchiveIpc->slot] = loadWaveArchiveIpc->archive;

			if(!loadWaveArchiveIpc->archive) break;

			consolePrintf("Loaded bank %d with %d waves\n", loadWaveArchiveIpc->slot, loadWaveArchiveIpc->archive->waves.size());
			consoleFlush();
		} break;

		case BaseIPC::CommandType::PlaySequence:
		{
			PlayTrackIPC *playTrackIpc = static_cast<PlayTrackIPC *>(ipc);
			playingSequence.allowedChannels = playTrackIpc->channelMask;
			playingSequence.sequenceVolume = playTrackIpc->sequenceVolume;
			playingSequence.PlaySequence(playTrackIpc->sequenceData, playTrackIpc->length, playTrackIpc->startPos);
		} break;

		case BaseIPC::CommandType::StopSequence:
		{
			playingSequence.AbortSequence(false);
		} break;

		case BaseIPC::CommandType::ReserveChannels:
		{
			ReserveChannelsIPC *reserveIpc = static_cast<ReserveChannelsIPC *>(ipc);
			externalChannelReservations = reserveIpc->reservations;
		}

		case BaseIPC::CommandType::AllocStreamPlayer:
		{
			assert(!streamPlayer);
			streamPlayer = std::make_unique<StreamPlayer>();
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
			streamPlayer->Init(initStreamIpc->encoding, initStreamIpc->stereo, initStreamIpc->timer);
		} break;

		case BaseIPC::CommandType::StopStream:
		{
			assert(streamPlayer);
			streamPlayer->Stop();
		} break;

		case BaseIPC::CommandType::StreamPushBlock:
		{
			assert(streamPlayer);
			auto pushBlockIpc = static_cast<StreamPushBlockIPC *>(ipc);

			std::unique_ptr<StreamBlock> block;
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
	void SequencePlayer::sendFifoSequenceStatus(const PlayingSequence &sequence) {
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
}