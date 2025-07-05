#include "nitroComposer/wave.h"

#include <cstdlib>

namespace NitroComposer {

LoadedWave::LoadedWave() : waveData(nullptr) {}
LoadedWave::LoadedWave(const Wave &wave) : Wave(wave) {}
LoadedWave::LoadedWave(LoadedWave &&old) : Wave(old), waveData(old.waveData) {
	old.waveData = nullptr;
}
LoadedWave::~LoadedWave() {
	free(waveData);
	waveData = nullptr;
}


void LoadedWaveArchive::Reset() {
	waves.clear();
	archiveId = 0xFFFF;
}

}