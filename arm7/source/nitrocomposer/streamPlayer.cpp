#include "streamPlayer.h"

#include <nds/arm7/console.h>
#include <nds/arm7/audio.h>

#include <cassert>

namespace NitroComposer {

	void StreamPlayer::Init(WaveEncoding encoding, bool stereo, std::uint16_t timer) {
		assert(encoding != WaveEncoding::Generated);
		this->encoding = encoding;
		this->stereo = stereo;
		this->timer = timer;
	}
	void StreamPlayer::Stop() {}
}