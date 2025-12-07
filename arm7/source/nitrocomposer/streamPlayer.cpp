#include "streamPlayer.h"
#include "sequencePlayer.h"

#include <nds/arm7/console.h>
#include <nds/arm7/audio.h>

#include <cassert>
#include <algorithm>
#include <cstring>

#define NITROCOMPOSER_LOG_STREAM

namespace NitroComposer {

	std::unique_ptr<StreamPlayer> streamPlayer;

	StreamPlayer::StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t timerId, std::uint8_t hwChannel) :
		playbackState(PlaybackState::Uninitialized),
		bufferedSampleCount(0),
		timerId(timerId),
		channels{
			StreamChannel(playbackBuffSize, hwChannel, StreamChannel::StereoChannel::Center),
			StreamChannel()
		}
	{
		assert(!streamPlayer);
		sequencePlayer.ReserveChannel(hwChannel);
	}
	StreamPlayer::StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t timerId, std::uint8_t hwChannelLeft, std::uint8_t hwChannelRight) :
		playbackState(PlaybackState::Uninitialized),
		bufferedSampleCount(0),
		timerId(timerId),
		channels{
			StreamChannel(playbackBuffSize, hwChannelLeft, StreamChannel::StereoChannel::Left),
			StreamChannel(playbackBuffSize, hwChannelRight, StreamChannel::StereoChannel::Right)
		}
	{
		assert(!streamPlayer);
		sequencePlayer.ReserveChannel(hwChannelLeft);
		sequencePlayer.ReserveChannel(hwChannelRight);
	}
	StreamPlayer::~StreamPlayer() {
		assert(streamPlayer.get() == this);
		sequencePlayer.UnreserveChannel(channels[0].GetHwChannel());
		if(channels[1].IsAllocated()) {
			sequencePlayer.UnreserveChannel(channels[1].GetHwChannel());
		}
	}

	void StreamPlayer::Init(WaveEncoding encoding, bool stereo, std::uint16_t sampleRate) {
		assert(encoding != WaveEncoding::Generated);
		this->streamEncoding = encoding;
		this->stereo = stereo;
		this->sampleRate = sampleRate;

		if(stereo) {
			assert(channels[1].IsAllocated());
			channels[0].SetStereoChannel(StreamChannel::StereoChannel::Left);
			channels[1].SetStereoChannel(StreamChannel::StereoChannel::Right);
		} else {
			channels[0].SetStereoChannel(StreamChannel::StereoChannel::Center);
		}

		switch(encoding) {
			case WaveEncoding::PCM8:
			case WaveEncoding::PCM16:
				this->playbackEncoding = encoding;
				break;
			case WaveEncoding::ADPCM:
				this->playbackEncoding = WaveEncoding::PCM16;
				break;
			default:
				assert(false);
		}
#ifdef NITROCOMPOSER_LOG_STREAM
		consolePuts("Stream Init.");
		consoleFlush();
#endif

		playbackState = PlaybackState::InitialBuffering;
	}
	void StreamPlayer::Stop(bool instantly) {

#ifdef NITROCOMPOSER_LOG_STREAM
		consolePuts("Stream Stop.");
		consoleFlush();
#endif

		if(instantly) {
			playbackState = PlaybackState::Stopped;
			setChannelRegisters();
			return;
		} else {
			switch(playbackState) {
			case PlaybackState::Uninitialized:
			case PlaybackState::Stopped:
			case PlaybackState::Stopping_FlushBlocks:
			case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
				return;
			case PlaybackState::InitialBuffering:
			case PlaybackState::BufferingUnderrun_LastBlock:
				playbackState = PlaybackState::Stopping_PlayoutRemainsOfBuffer;
				return;

			case PlaybackState::BufferingUnderrun_OutOfData:
				playbackState = PlaybackState::Stopped;
				return;

			case PlaybackState::Playing:
				playbackState = PlaybackState::Stopping_FlushBlocks;
				return;
			}

		}
	}

	void StreamPlayer::SetVolume(std::uint8_t volume) {
		assert(volume <= 127);
		this->volume = volume;
	}

	void StreamPlayer::SetPan(std::int8_t pan) {
		assert(pan > -63);
		assert(pan <= 64);
		this->pan = pan;
	}

	void StreamPlayer::AddBlock(std::unique_ptr<StreamBlock> &&block) {
		assert(block);
		assert(block->blockDataSize > 0);
		assert(block->blockSampleCount > 0);
		assert(block->blockData[0] != nullptr);
		if(stereo) assert(block->blockData[1] != nullptr);
		assert(block->blockId != 0);

		blocks.push_back(std::move(block));

		blockAdded();
	}

	void StreamPlayer::RetireBlock(std::uint32_t blockId) {
		RemoveBlock(blockId);
		sendFifoStreamRetireBlock(blockId);
	}
	void StreamPlayer::RemoveBlock(std::uint32_t blockId) {
		for (auto it = blocks.begin(); it != blocks.end(); ++it) {
			if ((*it)->blockId == blockId) {
				blocks.erase(it);
				return;
			}
		}
		assert(false);
	}

	StreamPlayer::StreamChannel::StreamChannel() : bufferSize(0), hwChannel(0xFF), stereoChannel(StereoChannel::Invalid) {}

	StreamPlayer::StreamChannel::StreamChannel(std::uint32_t bufferSize, std::uint8_t hwChannel, StereoChannel stereoChannel) :
		bufferSize(bufferSize),
		writePosition(0),
		hwChannel(hwChannel),
		stereoChannel(stereoChannel) {
		playbackBuffer = std::make_unique<std::uint8_t[]>(bufferSize);
	}
	StreamPlayer::StreamChannel::~StreamChannel() {}

	std::uint8_t StreamPlayer::StreamChannel::GetVolume() const {
		return streamPlayer->volume;
	}

	std::uint8_t StreamPlayer::StreamChannel::GetPan() const {
		int pan = streamPlayer->pan;
		switch(stereoChannel) {
		case StereoChannel::Center:
			break;
		case StereoChannel::Left:
			pan -= 64;
			break;
		case StereoChannel::Right:
			pan += 64;
			break;
		default:
			assert(false);
		}
		return std::clamp(pan, 0, 127);
	}

	void StreamPlayer::StreamChannel::UpdateRegisters() {
		assert(stereoChannel != StereoChannel::Invalid);
		assert(hwChannel < 16);

		REG_SOUNDXSAD(hwChannel) = (std::uint32_t)(playbackBuffer.get());
		REG_SOUNDXTMR(hwChannel) = SOUNDXTMR_FREQ(streamPlayer->sampleRate);
		REG_SOUNDXLEN(hwChannel) = bufferSize / 4;
		REG_SOUNDXPNT(hwChannel) = 0;

		std::uint32_t ctrlVal = 0;

		switch(streamPlayer->playbackState) {
			case PlaybackState::Uninitialized:
			case PlaybackState::Stopped:
			case PlaybackState::InitialBuffering:
				// Do nothing
				break;
			case PlaybackState::Playing:
			case PlaybackState::Stopping_FlushBlocks:
			case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
			case PlaybackState::BufferingUnderrun_LastBlock:
			case PlaybackState::BufferingUnderrun_OutOfData:
				ctrlVal |= SOUNDXCNT_ENABLE;
			break;
		}

		ctrlVal |= ((int)streamPlayer->playbackEncoding << 29);
		ctrlVal |= SOUNDXCNT_REPEAT;
		ctrlVal |= SOUNDXCNT_PAN(GetPan());
		ctrlVal |= SOUNDXCNT_VOL_MUL(GetVolume());

		SCHANNEL_CR(hwChannel) = ctrlVal;
	}

	bool StreamPlayer::StreamChannel::HWStillPlaying() const {
		return (SCHANNEL_CR(hwChannel) & SOUNDXCNT_ENABLE) == SOUNDXCNT_ENABLE;
	}

	void StreamPlayer::StreamChannel::NewBlock(const StreamBlock *block) {
		const std::uint8_t *data = GetBlockData(block);

		switch(streamPlayer->streamEncoding) {
		case WaveEncoding::ADPCM:
			adpcmDecoder.ReadChunkHeader(data);
			break;
		case WaveEncoding::PCM8:
		case WaveEncoding::PCM16:
			break;
		default:
			assert(0);
		}
	}

	void StreamPlayer::StreamChannel::FastForward(const StreamBlock *block, std::uint32_t startPos, std::uint32_t sampleCount) {
		const std::uint8_t *data = GetBlockData(block);

		switch(streamPlayer->streamEncoding) {
		case WaveEncoding::ADPCM: {
			const std::uint8_t *readStart = data + AdpcmDecoder::chunkHeaderSize + startPos / 2;
			adpcmDecoder.FastForwardData(readStart, sampleCount);
		} break;
		case WaveEncoding::PCM8:
		case WaveEncoding::PCM16:
			break;
		default:
			assert(0);
		}
	}

	void StreamPlayer::StreamChannel::AddToPlayback(const StreamBlock *block, std::uint32_t startPos, std::uint32_t sampleCount) {
		assert(sampleCount <= bufferSizeInSamples());

		size_t samplesLeftToWrite = sampleCount;

		while(samplesLeftToWrite) {
			size_t writeDistance = writeDistanceToEnd();
			size_t samplesToWrite = std::min(samplesLeftToWrite, writeDistance);
			writeToPlaybackBuffer(block, startPos, samplesToWrite);
			samplesLeftToWrite -= samplesToWrite;
			startPos += samplesToWrite;
			if(writeDistance == samplesToWrite) {
				writePosition = 0;
			}
		}
	}

	void StreamPlayer::StreamChannel::writeToPlaybackBuffer(const StreamBlock *block, std::uint32_t startPos, std::uint32_t sampleCount) {
		assert((writePosition + sampleCount) <= bufferSizeInSamples());

		const std::uint8_t *data=GetBlockData(block);

		switch(streamPlayer->streamEncoding) {
			case WaveEncoding::PCM8: {
				size_t dataSize = sampleCount;
				const std::uint8_t *readStart = data + startPos;
				std::uint8_t *writeStart = playbackBuffer.get() + writePosition;
				memcpy(writeStart, readStart, dataSize);
				break;
			}
			case WaveEncoding::PCM16: {
				size_t dataSize = sampleCount * sizeof(std::uint16_t);
				const std::uint8_t *readStart = data + startPos * sizeof(std::uint16_t);
				std::int16_t *writeStart = reinterpret_cast<std::int16_t *>(playbackBuffer.get()) + writePosition;
				memcpy(writeStart, readStart, dataSize);
				break;
			}
			case WaveEncoding::ADPCM: {
				assert((sampleCount % 2) == 0);
				const std::uint8_t * readStart = data + AdpcmDecoder::chunkHeaderSize + startPos/2;
				std::int16_t *writeStart = reinterpret_cast<std::int16_t *>(playbackBuffer.get()) + writePosition;
				adpcmDecoder.DecodeData(readStart, writeStart, sampleCount);
				
				break;
			}
			default:
				assert(false);
		}

		writePosition += sampleCount;
	}

	void StreamPlayer::StreamChannel::AddSilenceToPlayback(std::uint32_t sampleCount) {
		assert(sampleCount <= bufferSizeInSamples());
		size_t samplesLeftToWrite = sampleCount;
		while(samplesLeftToWrite) {
			size_t writeDistance = writeDistanceToEnd();
			size_t samplesToWrite = std::min(samplesLeftToWrite, writeDistance);
			switch(streamPlayer->playbackEncoding) {
				case WaveEncoding::PCM8: {
					std::uint8_t *writeStart = playbackBuffer.get() + writePosition;
					memset(writeStart, 0, samplesToWrite);
					break;
				}
				case WaveEncoding::PCM16: {
					std::int16_t *writeStart = reinterpret_cast<std::int16_t *>(playbackBuffer.get()) + writePosition;
					memset(writeStart, 0, samplesToWrite * sizeof(std::int16_t));
					break;
				}
				default:
					assert(0);
			}
			samplesLeftToWrite -= samplesToWrite;
			writePosition += samplesToWrite;
			if(writeDistance == samplesToWrite) {
				writePosition = 0;
			}
		}
	}

	const std::uint8_t *StreamPlayer::StreamChannel::GetBlockData(const NitroComposer::StreamBlock *block) {
		switch(stereoChannel) {
		case StereoChannel::Center:
		case StereoChannel::Left:
			return reinterpret_cast<const std::uint8_t *>(block->blockData[0]);
			break;
		case StereoChannel::Right:
			return reinterpret_cast<const std::uint8_t *>(block->blockData[1]);
			break;
		default:
			assert(0);
		}
	}

	size_t StreamPlayer::StreamChannel::bufferSizeInSamples() const {
		switch(streamPlayer->playbackEncoding) {
		case WaveEncoding::PCM8:
			return this->bufferSize;
		case WaveEncoding::PCM16:
			return this->bufferSize / sizeof(std::int16_t);
		default:
			assert(0);
		}
	}

	size_t StreamPlayer::StreamChannel::writeDistanceToEnd() const {
		return bufferSizeInSamples()-writePosition;
	}

	void StreamPlayer::completeStop() {
		playbackState = PlaybackState::Stopped;
		clearTimer();
		setChannelRegisters();
		sendFifoStreamPlaybackEnded();
	}

	void StreamPlayer::resumePlayback() {
		playbackState = PlaybackState::Playing;

		setChannelRegisters();
		setTimer();
	}

	void StreamPlayer::setChannelRegisters() {
		channels[0].UpdateRegisters();
		if(stereo) channels[1].UpdateRegisters();
	}

	void StreamPlayer::writeToChannels(std::uint32_t samplesLeftToWrite) {
		assert(samplesLeftToWrite > 0);

		while(samplesLeftToWrite) {

			if(!currentBlock) {
				getNextBlock();

				if(!currentBlock) switch(playbackState) {
				case PlaybackState::Stopping_FlushBlocks:
				case PlaybackState::InitialBuffering:
					break;
				case PlaybackState::Uninitialized:
				case PlaybackState::Stopped:
					return;
				case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
				case PlaybackState::BufferingUnderrun_OutOfData:
				case PlaybackState::BufferingUnderrun_LastBlock:
				{
					channels[0].AddSilenceToPlayback(samplesLeftToWrite);
					if(stereo) channels[1].AddSilenceToPlayback(samplesLeftToWrite);
				} return;
				case PlaybackState::Playing:
				default:
					assert(0);
					break;
				}
			}

			switch(playbackState) {
			case PlaybackState::Stopping_FlushBlocks:
			case PlaybackState::Playing:
			case PlaybackState::InitialBuffering:
				break;
			case PlaybackState::Uninitialized:
			case PlaybackState::Stopped:
			case PlaybackState::BufferingUnderrun_OutOfData:
			case PlaybackState::BufferingUnderrun_LastBlock:
			case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
			default:
				assert(0);
				break;
			}

			std::uint32_t samplesLeftInBlock = currentBlock->blockSampleCount - currentBlockReadPosition;
			std::uint32_t samplesToWrite = std::min(samplesLeftToWrite, samplesLeftInBlock);

			channels[0].AddToPlayback(currentBlock, currentBlockReadPosition, samplesToWrite);
			if(stereo) {
				channels[1].AddToPlayback(currentBlock, currentBlockReadPosition, samplesToWrite);
			}

			bufferedSampleCount += samplesToWrite;

			if(samplesToWrite == samplesLeftInBlock) {
				RetireBlock(currentBlock->blockId);
				getNextBlock();
			} else {
				currentBlockReadPosition += samplesToWrite;
			}

			samplesLeftToWrite -= samplesToWrite;
		}
	}

	void StreamPlayer::fastForwardToStartOfCurrentBlock() {
		channels[0].FastForward(currentBlock, 0, currentBlock->startPos);
		if(stereo) channels[1].FastForward(currentBlock, 0, currentBlock->startPos);

		currentBlockReadPosition = currentBlock->startPos;
	}

	void StreamPlayer::getNextBlock() {
		if(blocks.empty()) {
			handleOutOfBlocks();
			return;
		}

		currentBlock = blocks.front().get();
		currentBlockReadPosition = 0;

		channels[0].NewBlock(currentBlock);
		if(stereo) channels[1].NewBlock(currentBlock);

		fastForwardToStartOfCurrentBlock();
	}

	void StreamPlayer::blockAdded() {
		consolePrintf("New block. %d", (int)playbackState);
		consoleFlush();

		switch(playbackState) {
		case PlaybackState::InitialBuffering:
		case PlaybackState::BufferingUnderrun_OutOfData:
			bufferNextBlock();
			if(bufferedEnoughForPlayback()) {
				resumePlayback();
			}
			return;
		case PlaybackState::BufferingUnderrun_LastBlock:
			bufferNextBlock();
			if(bufferedEnoughForPlayback()) {
				playbackState = PlaybackState::Playing;
			}
			setChannelRegisters();
		case PlaybackState::Playing:
			return;
		case PlaybackState::Stopped:
		case PlaybackState::Stopping_FlushBlocks:
		case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
		case PlaybackState::Uninitialized:
			assert(0);
			return;
		}
	}

	void StreamPlayer::handleOutOfBlocks() {
		currentBlock = nullptr;
		currentBlockReadPosition = 0;

#ifdef NITROCOMPOSER_LOG_STREAM
		consolePuts("Stream OOB!");
		consoleFlush();
#endif

		switch(playbackState) {
		case PlaybackState::Playing:
			if(this->bufferedSampleCount > 0) {
				playbackState = PlaybackState::BufferingUnderrun_LastBlock;
			} else {
				playbackState = PlaybackState::BufferingUnderrun_OutOfData;
			}
			setChannelRegisters();
			sendFifoStreamOutOfData();
			break;

		case PlaybackState::Stopping_FlushBlocks:
			playbackState = PlaybackState::Stopping_PlayoutRemainsOfBuffer;
			setChannelRegisters();
			break;

		case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
			setChannelRegisters();
			break;
		case PlaybackState::InitialBuffering:
		case PlaybackState::BufferingUnderrun_LastBlock:
		case PlaybackState::BufferingUnderrun_OutOfData:
			break;
		case PlaybackState::Stopped:
		case PlaybackState::Uninitialized:
			assert(0);
		}
	}

	void StreamPlayer::bufferNextBlock() {
		getNextBlock();
		assert(currentBlock);
		std::uint32_t samplesLeftInBlock = currentBlock->blockSampleCount - currentBlockReadPosition;

		std::uint32_t samplesToBuffer = std::min(
			samplesLeftInBlock,
			channels[0].GetBufferSize() - bufferedSampleCount
		);

		writeToChannels(samplesToBuffer);
	}

	bool StreamPlayer::bufferedEnoughForPlayback() const {
		return bufferedSampleCount >= channels[0].GetBufferSize();
	}

	void StreamPlayer::setTimer() {
#ifdef NITROCOMPOSER_LOG_STREAM
		consolePuts("Stream Timer Start.");
		consoleFlush();
#endif
		timerStart(timerId, ClockDivider_256, TIMER_FREQ_SHIFT(sampleRate, 1, 8), timerCallback);
	}
	void StreamPlayer::clearTimer() {
#ifdef NITROCOMPOSER_LOG_STREAM
		consolePuts("Stream Timer Stop.");
		consoleFlush();
#endif

		timerStop(timerId);
	}
	void StreamPlayer::timerCallback() {
		streamPlayer->timerCallbackInstance();
	}
	void StreamPlayer::timerCallbackInstance() {
		if(bufferedSampleCount >= samplesPerTimerInterrupt) {
			bufferedSampleCount -= samplesPerTimerInterrupt;
		} else {
			bufferedSampleCount = 0;
		}
		writeToChannels(samplesPerTimerInterrupt);

		switch(playbackState) {
		case PlaybackState::Uninitialized:
		case PlaybackState::Stopped:
			assert(0);
			return;

		case PlaybackState::Playing:
			return;

		case PlaybackState::Stopping_FlushBlocks:
		case PlaybackState::Stopping_PlayoutRemainsOfBuffer:
			if(bufferedSampleCount == 0) {
				completeStop();
			}
			return;
		case PlaybackState::BufferingUnderrun_LastBlock:
			if(bufferedSampleCount == 0) {
				playbackState = PlaybackState::BufferingUnderrun_OutOfData;
				clearTimer();
			}
			return;

		case PlaybackState::InitialBuffering:
		case PlaybackState::BufferingUnderrun_OutOfData:
			return;
		}
	}
}