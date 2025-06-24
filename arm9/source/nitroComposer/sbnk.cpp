#include "sbnk.h"

#include "../binaryReader.h"

#include <nds/arm9/sassert.h>

SBNK::SBNK(const std::string &fileName) : sections(fileName) {
	Parse();
}

SBNK::SBNK(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
	Parse();
}

void SBNK::Parse() {
	auto section=sections.getSectionInfo("DATA");

	auto stream=sections.getSectionData(section);

	BinaryReader reader(std::move(stream));

	reader.skip(4 * 8);//"reserved for runtime use"
	std::uint32_t instrumentCount = reader.readLELong();

	instruments.reserve(instrumentCount);

	for(unsigned int instrumentIndex = 0; instrumentIndex < instrumentCount; ++instrumentIndex) {
		std::uint8_t type = reader.readByte();

		auto offset = reader.readLE24Bit();
		offset -= section->offset;
		printf("%i %li\n", type, offset);

		auto pos = reader.getPos();

		reader.setPos(offset);

		std::unique_ptr<SBNK::BaseInstrument> instrument = ParseInstrument(reader, type);
		instruments.emplace_back(std::move(instrument));

		reader.setPos(pos);
	}

	printf("Loaded %u instruments.\n",instruments.size());
}

std::unique_ptr<SBNK::BaseInstrument> SBNK::ParseInstrument(BinaryReader &reader, std::uint8_t type) {
	switch(type) {
	case 0:
		return std::unique_ptr<SBNK::BaseInstrument>();
	case 1:
	{
		std::unique_ptr<PCMInstrument> pcm = std::make_unique<PCMInstrument>();
		pcm->type = InstrumentType::PCM;
		pcm->wave = reader.readLEShort();
		pcm->archive = reader.readLEShort();

		pcm->baseNote = reader.readByte();
		pcm->attack = reader.readByte();
		pcm->decay = reader.readByte();
		pcm->sustain = reader.readByte();
		pcm->release = reader.readByte();
		pcm->pan = reader.readByte();
		return std::move(pcm);
	} break;
	case 2:
	{
		std::unique_ptr<PulseInstrument> pulse = std::make_unique<PulseInstrument>();
		pulse->type = InstrumentType::Pulse;
		pulse->duty = reader.readLEShort();

		pulse->baseNote = reader.readByte();
		pulse->attack = reader.readByte();
		pulse->decay = reader.readByte();
		pulse->sustain = reader.readByte();
		pulse->release = reader.readByte();
		pulse->pan = reader.readByte();
		return std::move(pulse);
	} break;
	case 3:
	{
		std::unique_ptr<NoiseInstrument> noise = std::make_unique<NoiseInstrument>();
		noise->type = InstrumentType::Noise;

		noise->baseNote = reader.readByte();
		noise->attack = reader.readByte();
		noise->decay = reader.readByte();
		noise->sustain = reader.readByte();
		noise->release = reader.readByte();
		noise->pan = reader.readByte();
		return std::move(noise);
	} break;

	case 16:
	case 17:
		return std::unique_ptr<SBNK::BaseInstrument>();
	default:
		sassert(0, "Unknown instrument type %i", type);
	}
}
