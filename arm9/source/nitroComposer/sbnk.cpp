#include "sbnk.h"

#include "../binaryReader.h"

#include <nds/arm9/sassert.h>

namespace NitroComposer {

	SBNK::SBNK(const std::string &fileName) : sections(fileName) {
		Parse();
	}

	SBNK::SBNK(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
		Parse();
	}

	void SBNK::Parse() {
		auto section = sections.getSectionInfo("DATA");

		auto stream = sections.getSectionData(section);

		BinaryReader reader(std::move(stream));

		reader.skip(4 * 8);//"reserved for runtime use"
		std::uint32_t instrumentCount = reader.readLELong();

		instruments.reserve(instrumentCount);

		for(unsigned int instrumentIndex = 0; instrumentIndex < instrumentCount; ++instrumentIndex) {
			std::uint8_t type = reader.readByte();

			auto offset = reader.readLE24Bit();
			offset -= section->offset;

			auto pos = reader.getPos();

			reader.setPos(offset);

			std::unique_ptr<SBNK::BaseInstrument> instrument = ParseInstrument(reader, type);
			instruments.emplace_back(std::move(instrument));

			reader.setPos(pos);
		}
	}

	std::unique_ptr<SBNK::BaseInstrument> SBNK::ParseInstrument(BinaryReader &reader, std::uint8_t type) {
		switch(type) {
		case 0:
			return std::unique_ptr<SBNK::BaseInstrument>();
		case 1:	{
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
		case 2:	{
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
		case 3:	{
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

		case 16: {
			std::unique_ptr<Drumkit> drums = std::make_unique<Drumkit>();
			drums->type = InstrumentType::Drumkit;
			drums->minNote = reader.readByte();
			drums->maxNote = reader.readByte();
			auto noteCount = drums->maxNote - drums->minNote + 1;
			drums->subInstruments.reserve(noteCount);
			for(std::uint8_t note = drums->minNote; note < drums->maxNote + 1; ++note) {
				uint8_t subType = reader.readByte();
				reader.skip(1);
				auto subInstrument = ParseInstrument(reader, subType);
				drums->subInstruments.emplace_back(std::move(subInstrument));
			}

			return std::move(drums);
		} break;
		case 17: {
			std::unique_ptr<SBNK::SplitInstrument> split = std::make_unique<SplitInstrument>();
			split->type = InstrumentType::Split;

			for(unsigned int region = 0; region < SplitInstrument::regionCount; ++region) {
				split->regions[region] = reader.readByte();
			}

			for(unsigned int region = 0; region < SplitInstrument::regionCount; ++region) {
				uint8_t subType = reader.readByte();
				reader.skip(1);
				auto subInstrument = ParseInstrument(reader, subType);
				split->subInstruments[region] = std::move(subInstrument);
			}

			return split;
		}
		default:
			sassert(0, "Unknown instrument type %i", type);
		}
	}

}