#ifndef NITROCOMPOSER_SBNK_H
#define NITROCOMPOSER_SBNK_H

#include "../sectionedFile.h"

class BinaryReader;

namespace NitroComposer {

	class SBNK {
	public:
		SBNK(const std::string &fileName);
		SBNK(std::unique_ptr<BinaryReadStream> &&stream);

		enum class InstrumentType {
			Null = 0,
			PCM = 1,
			Pulse = 2,
			Noise = 3,
			Drumkit = 16,
			Split = 17
		};

		class BaseInstrument {
		public:
			InstrumentType type;
		};

		class LeafInstrument : public BaseInstrument {
		public:
			uint8_t baseNote;
			uint8_t attack, sustain, decay, release;
			uint8_t pan;
		};

		class PCMInstrument : public LeafInstrument {
		public:
			uint16_t archive;
			uint16_t wave;
		};

		class PulseInstrument : public LeafInstrument {
		public:
			uint16_t duty;
		};

		class NoiseInstrument : public LeafInstrument {};

		class SplitInstrument : public BaseInstrument {
		public:
			std::uint8_t regions[8];
			std::unique_ptr<BaseInstrument> subInstruments[8];
		};
		class Drumkit : public BaseInstrument {
		public:
			uint8_t minNote;
			uint8_t maxNote;
			std::vector<std::unique_ptr<BaseInstrument>> subInstruments;
		};

	private:
		SectionedFile sections;

		std::vector<std::unique_ptr<BaseInstrument>> instruments;

		void Parse();
		std::unique_ptr<BaseInstrument> ParseInstrument(BinaryReader &, std::uint8_t type);
	};
}

#endif