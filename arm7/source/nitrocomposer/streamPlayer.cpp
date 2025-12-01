#include "streamPlayer.h"
#include "sequencePlayer.h"

#include <nds/arm7/console.h>
#include <nds/arm7/audio.h>

#include <cassert>
#include <algorithm>

namespace NitroComposer {

	std::unique_ptr<StreamPlayer> streamPlayer;

	StreamPlayer::StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t hwChannel) : 
		playbackState(PlaybackState::Uninitialized),
		channels{
			StreamChannel(playbackBuffSize, hwChannel, StreamChannel::StereoChannel::Center),
			StreamChannel()
		}
	{
		assert(!streamPlayer);
		sequencePlayer.ReserveChannel(hwChannel);
	}
	StreamPlayer::StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t hwChannelLeft, std::uint8_t hwChannelRight) :
		playbackState(PlaybackState::Uninitialized),
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

	void StreamPlayer::Init(WaveEncoding encoding, bool stereo, std::uint16_t timer) {
		assert(encoding != WaveEncoding::Generated);
		this->streamEncoding = encoding;
		this->stereo = stereo;
		this->timer = timer;

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

		playbackState = PlaybackState::InitialBuffering;
	}
	void StreamPlayer::Stop() {}

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
	}

	void StreamPlayer::FreeBlock(std::uint32_t blockId) {
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

	void StreamPlayer::StreamChannel::setRegisters() {
		assert(stereoChannel != StereoChannel::Invalid);
		assert(hwChannel < 16);

		REG_SOUNDXSAD(hwChannel) = (std::uint32_t)(playbackBuffer.get());
		REG_SOUNDXTMR(hwChannel) = streamPlayer->timer;
		REG_SOUNDXLEN(hwChannel) = bufferSize / 4;
		REG_SOUNDXPNT(hwChannel) = 0;

		std::uint32_t ctrlVal = 0;

		switch(streamPlayer->playbackState) {
			case PlaybackState::Uninitialized:
			case PlaybackState::Stopped:
			case PlaybackState::InitialBuffering:
			case PlaybackState::BufferingUnderrun:
				// Do nothing
				break;
			case PlaybackState::Playing:
				ctrlVal |= SOUNDXCNT_ENABLE;
			break;
		}

		ctrlVal |= ((int)streamPlayer->playbackEncoding << 29);
		ctrlVal |= SOUNDXCNT_REPEAT;
		ctrlVal |= SOUNDXCNT_PAN(GetPan());
		ctrlVal |= SOUNDXCNT_VOL_MUL(GetVolume());

		SCHANNEL_CR(hwChannel) = ctrlVal;
	}

	void StreamPlayer::updateChannels() {
		channels[0].Update();
		if(stereo) channels[1].Update();
	}

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
}