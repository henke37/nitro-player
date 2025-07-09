#include "swar.h"

#include "../binaryReader.h"
#include "../substream.h"

#include <nds/arm9/sassert.h>

namespace NitroComposer {

	SWAR::SWAR(const std::string &fileName) : sections(fileName) {
		Parse();
	}

	SWAR::SWAR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
		Parse();
	}

	const SWAR::WaveRecord &SWAR::GetWaveMetaData(unsigned int waveIndex) const {
		return waves.at(waveIndex);
	}


	std::unique_ptr<BinaryReadStream> SWAR::GetWaveData(const WaveRecord &waveInfo) const {
		auto section = sections.getSectionInfo("DATA");
		auto stream = sections.getSectionData(section);

		std::uint16_t sampleCount = waveInfo.loopStart + waveInfo.loopLength;
		size_t dataSize;

		switch(waveInfo.encoding) {
		case WaveEncoding::PCM8:
			dataSize = sampleCount * 1;
			break;
		case WaveEncoding::PCM16:
			dataSize = sampleCount * 2;
			break;
		case WaveEncoding::ADPCM:
			dataSize = 2 + sampleCount / 2;
			break;
		default:
			sassert(0, "Bad encoding %i", (int)waveInfo.encoding);
		}

		return std::make_unique<SubStream>(stream.release(), waveInfo.dataOffset, dataSize, true);
	}

	void SWAR::Parse() {
		auto section = sections.getSectionInfo("DATA");
		auto stream = sections.getSectionData(section);

		BinaryReader reader(std::move(stream));
		reader.skip(8 * 4);//"runtime reserved" nonsense
		std::uint32_t waveCount = reader.readLELong();
		waves.reserve(waveCount);

		for(std::uint32_t waveIndex = 0; waveIndex < waveCount; ++waveIndex) {
			std::uint32_t offset = reader.readLELong();
			offset -= section->offset;

			auto pos = reader.getPos();

			reader.setPos(offset);

			WaveRecord wave;

			wave.encoding = (WaveEncoding)reader.readByte();
			wave.loops = reader.readByte() != 0;
			wave.sampleRate = reader.readLEShort();
			wave.timerLen = reader.readLEShort();
			wave.loopStart = reader.readLEShort();
			wave.loopLength = reader.readLELong();
			wave.dataOffset = reader.getPos();

			waves.push_back(wave);

			reader.setPos(pos);
		}
	}

}