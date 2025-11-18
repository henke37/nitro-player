#include "sequencePlayer.h"

#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/arm7/console.h>
#include <nds/system.h>
#include <nds/arm7/audio.h>

#include "nitroComposer/ipc.h"

#include <cassert>

namespace NitroComposer {

	const size_t fifoBuffSize = 32;

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
			playingSequence.PlaySequence(playTrackIpc->sequenceData);
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

		default:
			assert(0);
		}
	}
	void SequencePlayer::sendFifoSequenceStatus(const PlayingSequence &sequence) {}
}