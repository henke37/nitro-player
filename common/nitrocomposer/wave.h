#ifndef NITROCOMPOSER_WAVE_H
#define NITROCOMPOSER_WAVE_H

#include <cstdint>
#include <vector>

namespace NitroComposer {

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
		void Reset();
	};

}

#endif