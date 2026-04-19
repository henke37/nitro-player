#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/arm9/cache.h>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

	SequencePlayer::SequencePlayer(SequencePlayerGroup *group) : group(group) {
		assert(group);

		playerId = musicEngine.registerPlayer(this);
		group->RegisterPlayer(this);

		SequencePlayerIPC buff;
		buff.command = BaseIPC::CommandType::AllocSequencePlayer;
		buff.playerId = playerId;

		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequencePlayerIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			std::uint32_t allocatedId = (fifoGetValue32(FIFO_NITRO_COMPOSER));
			assert(allocatedId == (std::uint32_t)playerId);
		}
	}
	SequencePlayer::~SequencePlayer() {
		KillSequence();

		SequencePlayerIPC buff;
		buff.command = BaseIPC::CommandType::DeallocSequencePlayer;
		buff.playerId = playerId;

		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequencePlayerIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			std::uint32_t allocatedId = (fifoGetValue32(FIFO_NITRO_COMPOSER));
			assert(allocatedId == (std::uint32_t)playerId);
		}

		group->UnregisterPlayer(this);

		musicEngine.unregisterPlayer(this);
	}

	void SequencePlayer::PlaySequence(const std::string &sequenceName) {
		unsigned int sequenceId = group->sdat->GetNamedSequenceIndex(sequenceName);
		PlaySequence(sequenceId);
	}

	void SequencePlayer::PlaySequence(unsigned int sequenceId) {
		group->LoadSequence(sequenceId);
		const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = group->GetLoadedSequenceInfo();

		auto &player = group->sdat->GetPlayerInfo(sequenceInfo->player);

		PlayTrackIPC buff;
		buff.command = BaseIPC::CommandType::PlaySequence;
		buff.playerId = playerId;
		buff.sequenceData = group->sequenceData.get();
		buff.startPos = 0;
		buff.length = group->sequenceDataSize;
		buff.channelMask = player->channelMask;
		buff.sequenceVolume = sequenceInfo->vol;
		//player priority is used to select which sequence gets to play at all.
		buff.sequencePriority = sequenceInfo->channelPriority;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(PlayTrackIPC), (u8 *)&buff);
		assert(success);

		isPlaying = true;
	}

	void SequencePlayer::AbortSequence() {
		SequencePlayerIPC buff;
		buff.command = BaseIPC::CommandType::StopSequence;
		buff.playerId = playerId;
		
		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequencePlayerIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			fifoGetValue32(FIFO_NITRO_COMPOSER);
		}

		isPlaying = false;
	}

	void SequencePlayer::KillSequence() {
		SequencePlayerIPC buff;
		buff.command = BaseIPC::CommandType::KillSequence;
		buff.playerId = playerId;

		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SequencePlayerIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			fifoGetValue32(FIFO_NITRO_COMPOSER);
		}

		isPlaying = false;
	}

	void SequencePlayer::sequenceEnded() {
		puts("Sequence ended");
		isPlaying = false;
	}

	void SequencePlayer::sendLoadBankIPC(const NitroComposer::InstrumentBank *loadedBank) {
		LoadBankIPC buff;
		buff.command = BaseIPC::CommandType::LoadBank;
		buff.playerId = playerId;
		buff.bank = loadedBank;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadBankIPC), (u8 *)&buff);
		assert(success);
	}

	void SequencePlayer::sendLoadWaveArchiveIPC(unsigned int archiveSlot, const NitroComposer::LoadedWaveArchive *loadedArchive) {
		LoadWaveArchiveIPC buff;
		buff.command = BaseIPC::CommandType::LoadWaveArchive;
		buff.playerId = playerId;
		buff.slot = archiveSlot;
		buff.archive = loadedArchive;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(LoadWaveArchiveIPC), (u8 *)&buff);
		assert(success);
	}

	void SequencePlayer::SetVar(std::uint8_t var, std::int16_t val) {
		SetVarIPC buff;
		buff.command = BaseIPC::CommandType::SetVar;
		buff.playerId = playerId;
		buff.var = var;
		buff.val = val;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)&buff);
		assert(success);
	}
	std::int16_t SequencePlayer::GetVar(std::uint8_t var) const {
		GetVarIPC buff;
		buff.command = BaseIPC::CommandType::GetVar;
		buff.playerId = playerId;
		buff.var = var;

		{
			FifoMutexLock lock;

			bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(GetVarIPC), (u8 *)&buff);
			assert(success);

			fifoWaitValue32Async(FIFO_NITRO_COMPOSER);
			return static_cast<std::int16_t> (fifoGetValue32(FIFO_NITRO_COMPOSER));
		}

	}

	void SequencePlayer::SetTrackMute(std::uint8_t trackId, bool mute) {
		MuteTrackIPC buff;
		buff.command = BaseIPC::CommandType::SetVar;
		buff.playerId = playerId;
		buff.trackId = trackId;
		buff.mute = mute;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(MuteTrackIPC), (u8 *)&buff);
		assert(success);
	}

	void SequencePlayer::SetAllowedChannelsOverride(std::uint16_t channels) {

		SetAllowedChannelsIPC buff;
		buff.command = BaseIPC::CommandType::SetAllowedChannels;
		buff.playerId = playerId;
		buff.channels = channels;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetAllowedChannelsIPC), (u8 *)&buff);
		assert(success);
	}
}