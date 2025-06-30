#ifndef NITROCOMPOSER_SWAR_H
#define NITROCOMPOSER_SWAR_H

#include "../sectionedFile.h"

namespace NitroComposer {

	class SWAR {
	public:
		SWAR(const std::string &fileName);
		SWAR(std::unique_ptr<BinaryReadStream> &&stream);

		enum class WaveEncoding {
			PCM8,
			PCM16,
			ADPCM,
			Generated
		};

		struct Wave {
			WaveEncoding encoding;

			bool loops;
			std::uint16_t loopStart;
			std::uint16_t loopLength;

			std::uint16_t sampleRate;
			std::uint16_t timerLen;

			std::uint32_t dataOffset;
		};

		const Wave &GetWaveMetaData(unsigned int waveIndex) const;
		std::unique_ptr<BinaryReadStream> GetWaveData(const Wave &waveInfo) const;
	private:
		SectionedFile sections;
		void Parse();

		std::vector<Wave> waves;
	};

}

#endif