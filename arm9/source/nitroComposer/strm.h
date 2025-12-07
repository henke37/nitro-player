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

		WaveEncoding GetEncoding() const { return encoding; }
		bool GetLoops() const { return loops; }
		std::uint8_t GetChannels() const { return channels; }
		std::uint16_t GetSampleRate() const { return sampleRate; }
		std::uint32_t GetLoopOffset() const { return loopOffset; }
		std::uint32_t GetSamples() const { return samples; }
		std::uint32_t GetBlockCount() const { return blockCount; }
		std::uint32_t GetBlockLen() const { return blockLen; }
		std::uint32_t GetBlockSamples() const { return blockSamples; }
		std::uint32_t GetLastBlockLen() const { return lastBlockLen; }
		std::uint32_t GetLastBlockSamples() const { return lastBlockSamples; }

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