#include "sseq.h"

#include "../fileStream.h"
#include "../binaryReader.h"
#include "../substream.h"

SSEQ::SSEQ(const std::string &fileName) : sections(fileName) {
}

SSEQ::SSEQ(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
}

std::unique_ptr<BinaryReadStream> SSEQ::GetCommandStream() const {
	auto section = sections.getSectionInfo("DATA");
	auto stream = sections.getSectionData(section);

	BinaryReader reader(stream.get(), false);
	auto offset = reader.readLELong();
	offset -= section->offset;//Why do you have to be like this?

	return std::make_unique<SubStream>(stream.release(), offset, 0xFFFFFFFF, true);
}