#ifndef NITROCOMPOSER_STREAMPLAYER_H
#define NITROCOMPOSER_STREAMPLAYER_H

#include <memory>

#include "nitroComposer/wave.h"

namespace NitroComposer {

	class StreamPlayer {
	public:
		StreamPlayer();
		~StreamPlayer();

		StreamPlayer(const StreamPlayer &) = delete;
		StreamPlayer &operator=(const StreamPlayer &) = delete;

		void Init(WaveEncoding encoding, bool stereo, std::uint16_t timer);
		void Stop();
	private:

	private:
		bool stereo;
		std::uint16_t timer;
		WaveEncoding encoding;

		void AddBlock();
		void FreeBlock();

		class StreamChannel {
		public:
			StreamChannel();
			~StreamChannel();
		private:
			std::unique_ptr<std::uint8_t[]> playbackBuffer;
			std::uint32_t bufferSize;
			std::uint32_t writePosition;
		};

		void sendFifoStreamRetireBlock(std::uint32_t blockId);
	};
}

#endif