#include "strm.h"

#include "../binaryReader.h"
#include "../substream.h"

#include <nds/arm9/sassert.h>

namespace NitroComposer {

	STRM::STRM(const std::string &fileName) : sections(fileName) {
		Parse();
	}

	STRM::STRM(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
		Parse();
	}

	std::unique_ptr<StreamBlock> STRM::ReadBlock(std::uint32_t blockIndex) {
		std::unique_ptr<BinaryReadStream> data = sections.getSectionData("DATA");
		sassert(data, "STRM has no DATA section!");

		sassert(blockIndex < blockCount, "BlockIndex %li out of bounds (%li)!",blockIndex,blockCount);

		bool isLastBlock = (blockIndex + 1) == blockCount;
		size_t blockSize = isLastBlock ? lastBlockLen : blockLen;
		std::uint32_t blockSamplesCount = isLastBlock ? lastBlockSamples : blockSamples;

		std::uint32_t blockOffset = blockIndex * blockLen * channels;

		std::unique_ptr<StreamBlock> block=std::make_unique<StreamBlock>();
		block->sampleCount = blockSamplesCount;
		block->dataSize = blockSize;
		block->startPos = 0;

		for(unsigned int channel = 0; channel < channels; ++channel) {
			data->setPos(blockOffset + blockSize * channel);
			block->blockData[channel] = std::make_unique<std::uint8_t[]>(blockSize);
			data->read(block->blockData[channel].get(), blockSize);
		}

		return block;
	}

	void STRM::Parse() {
		BinaryReader reader(sections.getSectionData("HEAD"));
		encoding = (WaveEncoding)reader.readByte();
		loops = reader.readByte() != 0;
		channels = reader.readByte();
		reader.skip(1);
		sampleRate = reader.readLEShort();
		timerResetVal = reader.readLEShort();
		loopOffset = reader.readLELong();
		samples = reader.readLELong();
		reader.skip(4);
		blockCount = reader.readLELong();
		blockLen = reader.readLELong();
		blockSamples = reader.readLELong();
		lastBlockLen = reader.readLELong();
		lastBlockSamples = reader.readLELong();

		sassert(channels > 0, "channels is Zero");
		sassert(channels <= 2, "channels is %d", channels);
		if(loops) sassert(loopOffset < samples, "Out of bounds loop index %lu, %lu max", loopOffset, samples);
	}

}