#ifndef NITROCOMPOSER_WAVE_H
#define NITROCOMPOSER_WAVE_H

#include <cstdint>
#include <vector>

namespace NitroComposer {

	enum class WaveEncoding {
		PCM8 = 0,
		PCM16 = 1,
		ADPCM = 2,
		Generated = 3
	};

	struct Wave {
		WaveEncoding encoding;

		bool loops;
		std::uint16_t loopStart;
		std::uint32_t loopLength;

		std::uint16_t sampleRate;
		std::uint16_t timerLen;
	};

	struct LoadedWave : Wave {
		LoadedWave();
		LoadedWave(const Wave &wave);
		LoadedWave(LoadedWave &&old);
		~LoadedWave();

		LoadedWave &operator=(LoadedWave &&);

		void *waveData;
	};

	struct LoadedWaveArchive {
		std::vector<LoadedWave> waves;
		std::uint16_t archiveId;

		void Reset();
	};

}

#endif