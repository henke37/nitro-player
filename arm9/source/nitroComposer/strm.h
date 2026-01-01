#ifndef NITROCOMPOSER_STRM_H
#define NITROCOMPOSER_STRM_H

#include "../sectionedFile.h"
#include "nitroComposer/wave.h"

namespace NitroComposer {

	struct StreamBlock {
	public:
		std::uint32_t blockId;
		size_t dataSize;
		std::uint32_t sampleCount;
		std::uint32_t startPos;
		std::unique_ptr<std::uint8_t[]> blockData[2];
	};

	class STRM {

	public:
		STRM(const std::string &fileName);
		STRM(std::unique_ptr<BinaryReadStream> &&stream);

		std::unique_ptr<StreamBlock> ReadBlock(std::uint32_t blockIndex);

		WaveEncoding GetEncoding() const noexcept { return encoding; }
		bool GetLoops() const noexcept { return loops; }
		std::uint8_t GetChannels() const noexcept { return channels; }
		std::uint16_t GetSampleRate() const noexcept { return sampleRate; }
		std::uint32_t GetLoopOffset() const noexcept { return loopOffset; }
		std::uint32_t GetSamples() const noexcept { return samples; }
		std::uint32_t GetBlockCount() const noexcept { return blockCount; }
		std::uint32_t GetBlockLen() const noexcept { return blockLen; }
		std::uint32_t GetBlockSamples() const noexcept { return blockSamples; }
		std::uint32_t GetLastBlockLen() const noexcept { return lastBlockLen; }
		std::uint32_t GetLastBlockSamples() const noexcept { return lastBlockSamples; }

	private:
		void Parse();
		SectionedFile sections;

		WaveEncoding encoding;
		bool loops;
		std::uint8_t channels;
		std::uint16_t sampleRate;
		std::uint32_t loopOffset;
		std::uint32_t samples;
		std::uint32_t blockCount;
		std::uint32_t blockLen;
		std::uint32_t blockSamples;
		std::uint32_t lastBlockLen;
		std::uint32_t lastBlockSamples;
	};

}

#endif