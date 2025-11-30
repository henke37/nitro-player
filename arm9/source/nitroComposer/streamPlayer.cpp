#include "streamPlayer.h"
#include "sequencePlayer.h"

#include <cassert>
#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/arm9/cache.h>

namespace NitroComposer {
	StreamPlayer::StreamPlayer() {
		assert(!musicEngine.currentStreamPlayer);
		musicEngine.currentStreamPlayer = this;

		BaseIPC ipc;
		ipc.command = BaseIPC::CommandType::AllocStreamPlayer;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(BaseIPC), (u8 *)&ipc);
		assert(success);
	}
	StreamPlayer::~StreamPlayer() {
		assert(musicEngine.currentStreamPlayer == this);
		musicEngine.currentStreamPlayer = nullptr;

		BaseIPC ipc;
		ipc.command = BaseIPC::CommandType::DeallocStreamPlayer;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(BaseIPC), (u8 *)&ipc);
		assert(success);
	}

	void StreamPlayer::SetVolume(std::uint8_t volume) {
		assert(volume <= 127);
		if(this->volume == volume) return;
		this->volume = volume;

		StreamVolumeIPC ipc;
		ipc.command = BaseIPC::CommandType::StreamSetVolume;
		ipc.volume = volume;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(StreamVolumeIPC), (u8 *)&ipc);
		assert(success);
	}

	void StreamPlayer::SetPan(std::uint8_t pan) {
		assert(pan <= 127);
		if(this->pan == pan) return;
		this->pan = pan;

		StreamPanIPC ipc;
		ipc.command = BaseIPC::CommandType::StreamSetPan;
		ipc.pan = pan;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(StreamPanIPC), (u8 *)&ipc);
		assert(success);
	}

	SingleStreamBlockSource::SingleStreamBlockSource(std::unique_ptr<STRM> &&stream) : stream(std::move(stream)) {
		assert(this->stream);
	}
	SingleStreamBlockSource::~SingleStreamBlockSource() {}
	std::unique_ptr<StreamBlock> SingleStreamBlockSource::GetNextBlock() {
		assert(stream);
		if(currentPos >= stream->GetSamples()) {
			if(!stream->GetLoops()) {
				currentPos = 0;
				return nullptr;
			}
			currentPos = stream->GetLoopOffset();
		}
		auto block = GetBlockAtAbsPos(currentPos);
		currentPos += block->sampleCount;
		return block;
	}
	SingleStreamBlockSource::SingleStreamBlockSource(const std::string &fileName)
		: SingleStreamBlockSource(std::make_unique<STRM>(fileName)) {}
	SingleStreamBlockSource::SingleStreamBlockSource(std::unique_ptr<BinaryReadStream> &&stream) 
		: SingleStreamBlockSource(std::make_unique<STRM>(std::move(stream))) {}

	SingleStreamBlockSource::ChunkPos SingleStreamBlockSource::ChunkForAbsolutePos(std::uint32_t absPos) const {
		assert(stream);
		assert(absPos < stream->GetSamples());
		ChunkPos pos;
		pos.chunkIndex = absPos / stream->GetBlockSamples();
		pos.sampleOffset = absPos % stream->GetBlockSamples();
		return pos;
	}

	std::unique_ptr<StreamBlock> SingleStreamBlockSource::GetBlockAtAbsPos(std::uint32_t absPos) const {
		assert(stream);
		auto chunkPos = ChunkForAbsolutePos(absPos);
		auto block = stream->ReadBlock(chunkPos.chunkIndex);
		block->startPos = chunkPos.sampleOffset;
		return block;
	}


	void StreamPlayer::StopStream() {
		if(playbackState == PlaybackState::Stopped) return;

		std::unique_ptr<StreamPlayerIPC> buff = std::make_unique<StreamPlayerIPC>();
		buff->command = BaseIPC::CommandType::StopStream;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(StreamPlayerIPC), (u8 *)buff.get());
		assert(success);
	}

	void StreamPlayer::PlayStream(unsigned int streamId) {
		auto &info = sdat->GetStreamInfo(streamId);
		PlayStream(info);
	}

	void StreamPlayer::PlayStream(const std::string &streamName) {
		auto &info = sdat->GetStreamInfo(streamName);
		PlayStream(info);
	}

	void StreamPlayer::PlayStream(const std::unique_ptr<StreamInfoRecord> &info) {
		auto strm = sdat->OpenStream(info);
		assert(strm->GetChannels() <= 2);

		this->blockSource = std::make_unique<SingleStreamBlockSource>(std::move(strm));

		StartPlayback();
	}

	void StreamPlayer::PlayStream(std::unique_ptr<IBlockSource> &&blockSource) {
		assert(blockSource);
		this->blockSource = std::move(blockSource);

		StartPlayback();
	}

	void StreamPlayer::addBlock(std::unique_ptr<StreamBlock> &&block) {
		block->blockId = nextBlockId++;
		if(nextBlockId==0) nextBlockId = 1;
		blocks.emplace_back(std::move(block));
	}

	void StreamPlayer::retireBlock(std::uint32_t blockId) {
		removeBlock(blockId);
		ensueEnoughQueuedBlocks();
	}

	void StreamPlayer::removeBlock(uint32_t blockId) {
		auto itr = blocks.begin();
		for(; itr != blocks.end(); ++itr) {
			if((*itr)->blockId == blockId) {
				blocks.erase(itr);
				return;
			}
		}
		assert(false);
	}

	std::uint32_t StreamPlayer::GetOutstandingSamples() const {
		std::uint32_t samples = 0;

		for(const auto &block : blocks) {
			assert(block->startPos < block->sampleCount);
			samples += block->sampleCount - block->startPos;
		}

		return samples;
	}

	void StreamPlayer::ensueEnoughQueuedBlocks() {
		
		switch(playbackState) {
			case PlaybackState::Stopped:
			case PlaybackState::Finishing:
				return;
			case PlaybackState::BufferUnderrun:
			case PlaybackState::Starting:
			case PlaybackState::Playing:
				break;
		}

		assert(blockSource);
		while(GetOutstandingSamples() < minQueuedSamples) {
			auto blockOwned = blockSource->GetNextBlock();
			if(!blockOwned) {
				playbackState = PlaybackState::Finishing;
				return;
			}

			assert(blockOwned->dataSize > 0);
			assert(blockOwned->sampleCount > 0);
			assert(blockOwned->startPos < blockOwned->sampleCount);

			StreamBlock *block = blockOwned.get();
			addBlock(std::move(blockOwned));
			sendPushBlockIPC(block);
		}

		if(playbackState == PlaybackState::BufferUnderrun) {
			playbackState = PlaybackState::Playing;
		}
	}

	void StreamPlayer::StartPlayback() {
		assert(blockSource);

		playbackState = PlaybackState::Starting;

		sendInitStreamIPC();
		ensueEnoughQueuedBlocks();

		playbackState = PlaybackState::Playing;
	}

	void StreamPlayer::sendInitStreamIPC() {
		InitStreamIPC ipc;

		ipc.command = BaseIPC::CommandType::InitStream;
		ipc.encoding = blockSource->GetEncoding();
		ipc.stereo = blockSource->GetChannels()>1;
		ipc.timer = blockSource->GetTimer();

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(InitStreamIPC), (u8 *)&ipc);
		assert(success);
	}
	void StreamPlayer::sendPushBlockIPC(const StreamBlock *block) {
		assert(block);
		assert(block->blockId != 0);
		assert(block->dataSize > 0);
		assert(block->sampleCount > 0);
		assert(block->startPos < block->sampleCount);

		StreamPushBlockIPC ipc;
		ipc.command = BaseIPC::CommandType::StreamPushBlock;
		ipc.blockId = block->blockId;
		ipc.blockDataSize = block->dataSize;
		ipc.blockSampleCount = block->sampleCount;
		ipc.startPos = block->startPos;

		for(unsigned int channel = 0; channel < 2; ++channel) {
			ipc.blockData[channel] = block->blockData[channel].get();

			DC_FlushRange(ipc.blockData[channel], block->dataSize);
		}
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(StreamPushBlockIPC), (u8 *)&ipc);
		assert(success);
	}
	void StreamPlayer::streamEnded() {
		assert(playbackState != PlaybackState::Stopped);
		playbackState = PlaybackState::Stopped;
	}
	void StreamPlayer::outOfData() {
		switch(playbackState) {
			case PlaybackState::Stopped:
			case PlaybackState::Finishing:
				return;
			case PlaybackState::BufferUnderrun:
				return;
			case PlaybackState::Starting:
				return;
			case PlaybackState::Playing:
				playbackState = PlaybackState::BufferUnderrun;
				break;
		}
	}
}