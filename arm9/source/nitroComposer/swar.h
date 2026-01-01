#ifndef NITROCOMPOSER_SWAR_H
#define NITROCOMPOSER_SWAR_H

#include "../sectionedFile.h"

#include "nitroComposer/wave.h"

namespace NitroComposer {

	class SWAR {
	public:
		SWAR(const std::string &fileName);
		SWAR(std::unique_ptr<BinaryReadStream> &&stream);

		struct WaveRecord : Wave {
			std::uint32_t dataOffset;
			size_t GetDataSize() const noexcept;
		};

		size_t GetWaveCount() const noexcept { return waves.size(); }

		const WaveRecord &GetWaveMetaData(unsigned int waveIndex) const;
		std::unique_ptr<BinaryReadStream> GetWaveData(const WaveRecord &waveInfo) const;
	private:
		SectionedFile sections;
		void Parse();

		std::vector<WaveRecord> waves;
	};

}

#endif