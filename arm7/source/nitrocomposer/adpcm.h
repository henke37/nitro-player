#ifndef NITROCOMPOSER_ADPCM_H
#define NITROCOMPOSER_ADPCM_H

#include <cstdint>
#include <cstddef>

#ifdef __INTELLISENSE__
#define ARMCODE
#else
#define ARMCODE __attribute__((target("arm")))
#endif

namespace NitroComposer {

	class AdpcmDecoder {
	public:
		AdpcmDecoder() = default;
		~AdpcmDecoder() = default;

		static constexpr size_t chunkHeaderSize = 4;
		static constexpr size_t samplesPerOctet = 2;

		void ReadChunkHeader(const std::uint8_t *inputData);
		void Init(std::int16_t predictor, int stepIndex);
		ARMCODE void FastForwardData(__restrict const std::uint8_t *inputData, size_t sampleCount);
		ARMCODE void DecodeData(__restrict const std::uint8_t *inputData, __restrict std::int16_t *outputData, size_t outputSampleCount);
		ARMCODE void DecodeBlock(__restrict const std::uint8_t *inputData, __restrict std::int16_t *outputData, size_t outputSampleCount);
	private:
		static const int indexTable[16];
		static const int stepTable[89];

		ARMCODE void parseNibble(int nibble);

		std::int_fast16_t predictor = 0;
		std::int_fast8_t stepIndex = 0;
		std::int_fast8_t step = 0;
	};
	
}

#endif
