#ifndef NITROCOMPOSER_STREAMPLAYER_H
#define NITROCOMPOSER_STREAMPLAYER_H

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
		};
	};
}

#endif