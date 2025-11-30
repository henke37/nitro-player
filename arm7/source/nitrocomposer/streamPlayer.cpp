#include "streamPlayer.h"
#include "sequencePlayer.h"

#include <nds/arm7/console.h>
#include <nds/arm7/audio.h>

#include <cassert>

namespace NitroComposer {

	std::unique_ptr<StreamPlayer> streamPlayer;

	StreamPlayer::StreamPlayer() : playbackState(PlaybackState::Uninitialized) {
		assert(!streamPlayer);
	}
	StreamPlayer::~StreamPlayer() {
		assert(streamPlayer.get() == this);
	}

	void StreamPlayer::Init(WaveEncoding encoding, bool stereo, std::uint16_t timer) {
		assert(encoding != WaveEncoding::Generated);
		this->streamEncoding = encoding;
		this->stereo = stereo;
		this->timer = timer;

		switch(encoding) {
			case WaveEncoding::PCM8:
			case WaveEncoding::PCM16:
				this->playbackEncoding = encoding;
				break;
			case WaveEncoding::ADPCM:
				this->playbackEncoding = WaveEncoding::PCM8;
				break;
			default:
				assert(false);
		}

		playbackState = PlaybackState::InitialBuffering;
	}
	void StreamPlayer::Stop() {}

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

	StreamPlayer::StreamChannel::StreamChannel() : writePosition(0) {
		bufferSize = 4096;
		playbackBuffer = std::make_unique<std::uint8_t[]>(bufferSize);
	}
	StreamPlayer::StreamChannel::~StreamChannel() {}

	void StreamPlayer::StreamChannel::setRegisters() {
		REG_SOUNDXSAD(hwChannel) = (std::uint32_t)(playbackBuffer.get());
		REG_SOUNDXTMR(hwChannel) = streamPlayer->timer;
		REG_SOUNDXLEN(hwChannel) = bufferSize / 4;
		REG_SOUNDXPNT(hwChannel) = 0;

		SCHANNEL_CR(hwChannel) = SOUNDXCNT_ENABLE |
			((int)streamPlayer->playbackEncoding << 29) |
			SOUNDXCNT_REPEAT |
			SOUNDXCNT_PAN(GetPan()) |
			SOUNDXCNT_VOL_MUL(GetVolume());
	}
}