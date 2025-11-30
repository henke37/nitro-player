#ifndef NITROCOMPOSER_STREAMPLAYER_H
#define NITROCOMPOSER_STREAMPLAYER_H

#include <memory>

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
		StreamPlayer();
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

		void Init(WaveEncoding encoding, bool stereo, std::uint16_t timer);
		void Stop();

		void SetVolume(std::uint8_t volume);
		void SetPan(std::uint8_t pan);
	private:

	private:
		bool stereo;
		PlaybackState playbackState;
		std::uint16_t timer;
		WaveEncoding streamEncoding;
		WaveEncoding playbackEncoding;

		void AddBlock(std::unique_ptr<StreamBlock> &&block);
		void FreeBlock(std::uint32_t blockId);
		void RemoveBlock(std::uint32_t blockId);
		std::vector<std::unique_ptr<StreamBlock>> blocks;

		class StreamChannel {
		public:
			StreamChannel();
			~StreamChannel();

			enum class StereoChannel : std::uint8_t {
				Center = 0,
				Left = 1,
				Right = 2
			};

		private:
			std::unique_ptr<std::uint8_t[]> playbackBuffer;
			std::uint32_t bufferSize;
			std::uint32_t writePosition;

			StereoChannel stereoChannel;

			std::uint8_t hwChannel;

			std::uint8_t GetVolume() const;
			std::uint8_t GetPan() const;

			void setRegisters();
		};

		std::uint8_t volume = 127;
		std::uint8_t pan = 64;

		void sendFifoStreamRetireBlock(std::uint32_t blockId);
		void sendFifoStreamOutOfData();

		friend class StreamChannel;
		friend class SequencePlayer;
	};

	extern std::unique_ptr<StreamPlayer> streamPlayer;
}

#endif