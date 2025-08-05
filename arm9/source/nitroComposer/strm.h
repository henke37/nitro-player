#ifndef NITROCOMPOSER_STRM_H
#define NITROCOMPOSER_STRM_H

#include "../sectionedFile.h"
#include "nitroComposer/wave.h"

namespace NitroComposer {

	class STRM {

	public:
		STRM(const std::string &fileName);
		STRM(std::unique_ptr<BinaryReadStream> &&stream);

		void ReadBlock() const;

	private:
		void Parse();
		SectionedFile sections;

		WaveEncoding encoding;
		bool loops;
		std::uint8_t channels;
		std::uint16_t sampleRate;
		std::uint16_t timer;
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