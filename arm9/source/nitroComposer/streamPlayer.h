#ifndef NITROCOMPOSER_STREAMPLAYER_H
#define NITROCOMPOSER_STREAMPLAYER_H

#include <cstdint>
#include <vector>

#include "nitroComposer/infoRecords.h"
#include "nitroComposer/ipc.h"

#include "sdatFile.h"
#include "strm.h"

namespace NitroComposer {

	class IBlockSource {
	public:
		virtual ~IBlockSource() = default;
	};

	class SingleStreamBlockSource : public IBlockSource {
	public:
		SingleStreamBlockSource(const std::string &fileName);
		SingleStreamBlockSource(std::unique_ptr<BinaryReadStream> &&stream);
		SingleStreamBlockSource(std::unique_ptr<STRM> &&stream);
		~SingleStreamBlockSource();
	private:
		std::unique_ptr<STRM> stream;
	};

	class PlaylistBlockSource : public IBlockSource {
	public:
		PlaylistBlockSource();
		~PlaylistBlockSource();
	private:
		std::vector<std::unique_ptr<STRM>> streams;
	};

	class StreamPlayer {
	public:
		StreamPlayer();
		~StreamPlayer();

		void SetSdat(const SDatFile *sdat);

		void PlayStream(unsigned int streamId);
		void PlayStream(const std::string &streamName);
		void PlayStream(const std::unique_ptr<StreamInfoRecord> &);

		void PlayStream(std::unique_ptr<IBlockSource> blockSource);
	private:
		const SDatFile *sdat;
		std::unique_ptr<IBlockSource> blockSource;

		bool isPlaying = false;

		void sendInitStreamIPC(WaveEncoding encoding, bool stereo, std::uint16_t timer);
	};
};
#endif