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

		virtual std::unique_ptr<StreamBlock> GetNextBlock() = 0;

		virtual WaveEncoding GetEncoding() const = 0;
		virtual std::uint8_t GetChannels() const = 0;
		virtual std::uint16_t GetSampleRate() const = 0;
		virtual std::uint16_t GetTimer() const = 0;
	};

	class SingleStreamBlockSource : public IBlockSource {
	public:
		SingleStreamBlockSource(const std::string &fileName);
		SingleStreamBlockSource(std::unique_ptr<BinaryReadStream> &&stream);
		SingleStreamBlockSource(std::unique_ptr<STRM> &&stream);
		~SingleStreamBlockSource();

		std::unique_ptr<StreamBlock> GetNextBlock() override;

		WaveEncoding GetEncoding() const override { return stream->GetEncoding(); }
		std::uint8_t GetChannels() const override { return stream->GetChannels(); }
		std::uint16_t GetSampleRate() const override { return stream->GetSampleRate(); }
		std::uint16_t GetTimer() const override { return stream->GetTimer(); }
	private:
		std::unique_ptr<STRM> stream;

	};

	class PlaylistBlockSource : public IBlockSource {
	public:
		PlaylistBlockSource();
		~PlaylistBlockSource();

		std::unique_ptr<StreamBlock> GetNextBlock() override;

		WaveEncoding GetEncoding() const override;
		std::uint8_t GetChannels() const override;
		std::uint16_t GetSampleRate() const override;
		std::uint16_t GetTimer() const override;

		void AddStream(std::unique_ptr<STRM> &&stream);
	private:
		std::vector<std::unique_ptr<STRM>> streams;
	};

	class StreamPlayer {
	public:
		StreamPlayer();
		~StreamPlayer();

		void SetSdat(const SDatFile *sdat);

		void StopStream();

		void PlayStream(unsigned int streamId);
		void PlayStream(const std::string &streamName);
		void PlayStream(const std::unique_ptr<StreamInfoRecord> &);

		void PlayStream(std::unique_ptr<IBlockSource> &&blockSource);
	private:
		const SDatFile *sdat;
		std::unique_ptr<IBlockSource> blockSource;

		bool isPlaying = false;

		void StartPlayback();

		void sendInitStreamIPC();
	};
};
#endif