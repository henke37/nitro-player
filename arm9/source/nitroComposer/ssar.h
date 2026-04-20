#ifndef NITROCOMPOSER_SSAR_H
#define NITROCOMPOSER_SSAR_H

#include <vector>
#include <cstdint>

#include "../sectionedFile.h"

#include "nitroComposer/infoRecords.h"

namespace NitroComposer {

	class SSAR {
	public:
		SSAR(const std::string &fileName);
		SSAR(std::unique_ptr<BinaryReadStream> &&stream);

		std::unique_ptr<BinaryReadStream> GetCommandStream() const;
		const std::unique_ptr<SequenceInfoRecord> &GetSequenceArchiveInfo(unsigned int archiveId) const;
		unsigned int GetSubSequenceCount() const { return sequenceInfos.size(); }

	private:
		SectionedFile sections;

		std::uint32_t commandStreamOffset;

		std::vector<std::unique_ptr<SequenceInfoRecord>> sequenceInfos;

		void Parse();
	};
	
}

#endif