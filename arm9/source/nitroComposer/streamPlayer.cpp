#include "streamPlayer.h"

#include <cassert>
#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>

namespace NitroComposer {
	StreamPlayer::StreamPlayer() {}
	StreamPlayer::~StreamPlayer() {}


	SingleStreamBlockSource::SingleStreamBlockSource(std::unique_ptr<STRM> &&stream) : stream(std::move(stream)) {
		assert(this->stream);
	}
	SingleStreamBlockSource::~SingleStreamBlockSource() {}
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


	void StreamPlayer::StopStream() {
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
		blocks.emplace_back(std::move(block));
	}

	void StreamPlayer::retireBlock(std::uint32_t blockId) {
		auto itr = blocks.begin();
		for(; itr != blocks.end(); ++itr) {
			if((*itr)->blockId == blockId) {
				blocks.erase(itr);
				return;
			}
		}
		assert(false);
	}

	void StreamPlayer::StartPlayback() {
		assert(blockSource);

		sendInitStreamIPC();
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
	void StreamPlayer::sendPushBlockIPC(const std::unique_ptr<StreamBlock> &block) {
		assert(block);

		StreamPushBlockIPC ipc;
		ipc.command = BaseIPC::CommandType::StreamPushBlock;
		ipc.blockId = block->blockId;
		ipc.blockDataSize = block->dataSize;
		ipc.blockSampleCount = block->sampleCount;
		ipc.startPos = block->startPos;

		for(unsigned int channel = 0; channel < 2; ++channel) {
			ipc.blockData[channel] = block->blockData[channel].get();
		}
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(StreamPushBlockIPC), (u8 *)&ipc);
		assert(success);
	}
}