#ifndef NITROCOMPOSER_STREAMPLAYER_H
#define NITROCOMPOSER_STREAMPLAYER_H

#include <memory>

#include "adpcm.h"

#include "nitroComposer/wave.h"

namespace NitroComposer {

	struct StreamBlock {
		std::uint32_t blockId;
		std::uint32_t blockDataSize;
		std::uint32_t blockSampleCount;
		std::uint32_t startPos;
		void *blockData[2];
	};

	class StreamPlayer {
	public:
		StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t timerId, std::uint8_t hwChannel);
		StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t timerId, std::uint8_t hwChannelLeft, std::uint8_t hwChannelRight);
		~StreamPlayer();

		enum class PlaybackState : std::uint8_t {
			Uninitialized=0,
			Stopped,
			Playing,
			InitialBuffering,
			BufferingUnderrun
		};

		StreamPlayer(const StreamPlayer &) = delete;
		StreamPlayer &operator=(const StreamPlayer &) = delete;

		void Init(WaveEncoding encoding, bool stereo, std::uint16_t timerResetVal);
		void Stop();

		void SetVolume(std::uint8_t volume);
		void SetPan(std::int8_t pan);
	private:

	private:
		bool stereo;
		PlaybackState playbackState;
		std::uint16_t timerResetVal;
		WaveEncoding streamEncoding;
		WaveEncoding playbackEncoding;

		std::uint8_t volume = 127;
		std::uint8_t pan = 64;

		const std::uint8_t timerId;

		void AddBlock(std::unique_ptr<StreamBlock> &&block);
		void RetireBlock(std::uint32_t blockId);
		void RemoveBlock(std::uint32_t blockId);
		std::vector<std::unique_ptr<StreamBlock>> blocks;

		const StreamBlock *currentBlock;
		std::uint32_t currentBlockReadPosition;//in samples

		void getNextBlock();

		class StreamChannel {
		public:
			enum class StereoChannel : std::uint8_t {
				Center = 0,
				Left = 1,
				Right = 2,
				Invalid = 0xFF
			};

			StreamChannel();
			StreamChannel(std::uint32_t bufferSize, std::uint8_t hwChannel, StereoChannel stereoChannel);
			~StreamChannel();

			std::uint8_t GetHwChannel() const { return hwChannel; }
			bool IsAllocated() const { return hwChannel < 16; }

			void SetStereoChannel(StereoChannel channel) { stereoChannel = channel; }

			void NewBlock(const StreamBlock *block);

			void FastForward(const StreamBlock *block, std::uint32_t startPos, std::uint32_t sampleCount);

			void AddToPlayback(const StreamBlock *block, std::uint32_t startPos, std::uint32_t sampleCount);

			const uint8_t *GetBlockData(const NitroComposer::StreamBlock *block);

		private:
			std::unique_ptr<std::uint8_t[]> playbackBuffer;
			std::uint32_t bufferSize;//in octets
			std::uint32_t writePosition;//in samples

			AdpcmDecoder adpcmDecoder;

			std::uint8_t hwChannel;

			StereoChannel stereoChannel;

			std::uint8_t GetVolume() const;
			std::uint8_t GetPan() const;

			void setRegisters();

			void writeToPlaybackBuffer(const StreamBlock *block, std::uint32_t startPos, std::uint32_t sampleCount);

			size_t bufferSizeInSamples() const;
			size_t writeDistanceToEnd() const;
		};

		StreamChannel channels[2];

		void writeToChannels(std::uint32_t sampleCount);
		void fastForwardToStartOfCurrentBlock();

		void sendFifoStreamRetireBlock(std::uint32_t blockId);
		void sendFifoStreamOutOfData();

		void setTimer();
		void clearTimer();

		static void timerCallback();

		friend class StreamChannel;
		friend class SequencePlayer;
	};

	extern std::unique_ptr<StreamPlayer> streamPlayer;
}

#endif