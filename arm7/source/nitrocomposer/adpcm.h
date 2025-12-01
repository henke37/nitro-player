#ifndef NITROCOMPOSER_ADPCM_H
#define NITROCOMPOSER_ADPCM_H

#include <cstdint>
#include <cstddef>

namespace NitroComposer {

	class AdpcmDecoder {
	public:
		AdpcmDecoder() = default;
		~AdpcmDecoder() = default;

		static const size_t chunkSize = 4;
		void ReadChunkHeader(const std::uint8_t *inputData);
		void Init(std::int16_t predictor, int stepIndex);
		void DecodeData(const std::uint8_t *inputData, std::int16_t *outputData, size_t outputSampleCount);
		void DecodeBlock(const std::uint8_t *inputData, std::int16_t *outputData, size_t outputSampleCount);
	private:
		static const int indexTable[16];
		static const int stepTable[89];

		void parseNibble(int nibble);

		std::int_fast16_t predictor = 0;
		std::int_fast8_t stepIndex = 0;
		std::int_fast8_t step = 0;
	};
	
}

#endif
