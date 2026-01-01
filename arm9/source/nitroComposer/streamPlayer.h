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

		virtual WaveEncoding GetEncoding() const noexcept = 0;
		virtual std::uint8_t GetChannels() const noexcept = 0;
		virtual std::uint16_t GetSampleRate() const noexcept = 0;
	};

	class SingleStreamBlockSource : public IBlockSource {
	public:
		SingleStreamBlockSource(const std::string &fileName);
		SingleStreamBlockSource(std::unique_ptr<BinaryReadStream> &&stream);
		SingleStreamBlockSource(std::unique_ptr<STRM> &&stream);
		~SingleStreamBlockSource();

		std::uint32_t GetCurrentPos() const noexcept { return currentPos; }

		std::unique_ptr<StreamBlock> GetNextBlock() override;

		WaveEncoding GetEncoding() const noexcept override { return stream->GetEncoding(); }
		std::uint8_t GetChannels() const noexcept override { return stream->GetChannels(); }
		std::uint16_t GetSampleRate() const noexcept override { return stream->GetSampleRate(); }
	private:
		std::unique_ptr<STRM> stream;
		std::uint32_t currentPos = 0;

		struct ChunkPos {
			std::uint32_t chunkIndex;
			std::uint32_t sampleOffset;
		};

		ChunkPos ChunkForAbsolutePos(std::uint32_t absPos) const;
		std::unique_ptr<StreamBlock> GetBlockAtAbsPos(std::uint32_t absPos) const;
	};

	class PlaylistBlockSource : public IBlockSource {
	public:
		PlaylistBlockSource();
		~PlaylistBlockSource();

		std::unique_ptr<StreamBlock> GetNextBlock() override;

		WaveEncoding GetEncoding() const noexcept override;
		std::uint8_t GetChannels() const noexcept override;
		std::uint16_t GetSampleRate() const noexcept override;

		void AddStream(std::unique_ptr<STRM> &&stream);
	private:
		std::vector<std::unique_ptr<STRM>> streams;
	};

	class StreamPlayer {
	public:
		StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t timerId, std::uint8_t hwChannel);
		StreamPlayer(std::uint32_t playbackBuffSize, std::uint8_t timerId, std::uint8_t hwChannelLeft, std::uint8_t hwChannelRight);
		StreamPlayer(const StreamPlayer &) = delete;
		~StreamPlayer();

		void SetSdat(const SDatFile *sdat);

		bool IsPlaying() const;

		void SetVolume(std::uint8_t volume);
		std::uint8_t GetVolume() const { return volume; }
		void SetPan(std::int8_t pan);
		std::int8_t GetPan() const { return pan; }

		void StopStream(bool instant);

		void PlayStream(unsigned int streamId);
		void PlayStream(const std::string &streamName);
		void PlayStream(const std::unique_ptr<StreamInfoRecord> &);

		void PlayStream(std::unique_ptr<IBlockSource> &&blockSource);
	private:
		const SDatFile *sdat;
		std::unique_ptr<IBlockSource> blockSource;

		std::uint32_t nextBlockId = 1;
		std::vector<std::unique_ptr<StreamBlock>> blocks;
		void addBlock(std::unique_ptr<StreamBlock> &&block);
		void retireBlock(std::uint32_t blockId);

		void removeBlock(uint32_t blockId);

		std::uint32_t GetOutstandingSamples() const;
		void ensueEnoughQueuedBlocks();
		static const std::uint32_t minQueuedSamples = 4096;

		enum class PlaybackState : std::uint8_t {
			Stopped=0,
			Starting,
			Playing,
			BufferUnderrun,
			Finishing
		};
		PlaybackState playbackState = PlaybackState::Stopped;

		std::uint8_t volume = 127;
		std::int8_t pan = 0;

		void StartPlayback();

		void sendInitStreamIPC();
		void sendPushBlockIPC(const StreamBlock *block);

		void streamEnded();
		void outOfData();

		friend class MusicEngine;
	};
};
#endif