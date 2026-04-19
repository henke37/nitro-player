#include "ssar.h"

#include "../binaryReader.h"
#include "../substream.h"

#include <nds/arm9/sassert.h>

namespace NitroComposer {

	SSAR::SSAR(const std::string &fileName) : sections(fileName) {
		Parse();
	}

	SSAR::SSAR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
		Parse();
	}



	const std::unique_ptr<SequenceInfoRecord> &SSAR::GetSequenceArchiveInfo(unsigned int archiveId) const {
		sassert(archiveId < sequenceInfos.size(), "Sequence archive %u past end of sequence archive info list", archiveId);
		return sequenceInfos[archiveId];
	}

	void SSAR::Parse() {
		auto section = sections.getSectionInfo("DATA");
		auto stream = sections.getSectionData(section);

		BinaryReader reader(stream.get(), false);
		commandStreamOffset = reader.readLELong();
		commandStreamOffset -= section->offset;//Why do you have to be like this?

		std::uint32_t sequenceCount = reader.readLELong();

		for(std::uint32_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex) {

			std::unique_ptr<SequenceInfoRecord> record = std::make_unique<SequenceInfoRecord>();
			record->startOffset = reader.readLELong();
			record->bankId = reader.readLEShort();
			record->vol = reader.readByte();
			record->channelPriority = reader.readByte();
			record->playerPriority = reader.readByte();
			record->player = reader.readByte();
			sequenceInfos.emplace_back(std::move(record));
		}
	}
	
	std::unique_ptr<BinaryReadStream> SSAR::GetCommandStream() const {
		auto section = sections.getSectionInfo("DATA");
		auto stream = sections.getSectionData(section);

		return std::make_unique<SubStream>(stream.release(), commandStreamOffset, 0xFFFFFFFF, true);
	}
}