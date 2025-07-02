#ifndef NITROCOMPOSER_SBNK_H
#define NITROCOMPOSER_SBNK_H

#include "../sectionedFile.h"

#include "nitroComposer/bank.h"

class BinaryReader;


namespace NitroComposer {

	class SBNK : public InstrumentBank {
	public:
		SBNK(const std::string &fileName);
		SBNK(std::unique_ptr<BinaryReadStream> &&stream);

	private:
		SectionedFile sections;

		void Parse();
		std::unique_ptr<BaseInstrument> ParseInstrument(BinaryReader &, std::uint8_t type);
	};
}

#endif