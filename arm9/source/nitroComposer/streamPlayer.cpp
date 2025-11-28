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
		PlayStream(std::make_unique<SingleStreamBlockSource>(std::move(strm)));
	}
	void StreamPlayer::sendInitStreamIPC(WaveEncoding encoding, bool stereo, std::uint16_t timer) {
		InitStreamIPC ipc;

		ipc.command = BaseIPC::CommandType::InitStream;
		ipc.encoding = encoding;
		ipc.stereo = stereo;
		ipc.timer = timer;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(InitStreamIPC), (u8 *)&ipc);
		assert(success);
	}
}