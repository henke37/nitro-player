#include "sectionedFile.h"

#include "fileStream.h"
#include "substream.h"
#include "binaryReader.h"

#include <cassert>

SectionedFile::SectionedFile(const std::string &fileName) {
	stream=std::make_unique<FileReadStream>(fileName);
	
	readSectionHeaders();
}

SectionedFile::SectionedFile(std::unique_ptr<BinaryReadStream> &&stream) : stream(std::move(stream)) {
	assert(this->stream);
	readSectionHeaders();
}

SectionedFile::~SectionedFile() {
}

const SectionedFile::Section *SectionedFile::getSectionInfo(size_t sectionOffset) const {
	for(auto itr=sections.cbegin();itr!=sections.cend();++itr) {
		if(itr->offset==sectionOffset) return &*itr;
	}
	return nullptr;
}

const SectionedFile::Section *SectionedFile::getSectionInfo(const std::string &sectionName) const {
	for(auto itr=sections.cbegin();itr!=sections.cend();++itr) {
		if(itr->name==sectionName) return &*itr;
	}
	return nullptr;
}

std::unique_ptr<BinaryReadStream> SectionedFile::getSectionData(size_t sectionOffset) const {
	const SectionedFile::Section *sect=getSectionInfo(sectionOffset);
	
	if(!sect) return nullptr;
	
	return readSectionData(sect);
}

std::unique_ptr<BinaryReadStream> SectionedFile::getSectionData(const std::string &sectionName) const {
	const SectionedFile::Section *sect=getSectionInfo(sectionName);
	
	if(!sect) return nullptr;
	
	return readSectionData(sect);
}

std::unique_ptr<BinaryReadStream> SectionedFile::readSectionData(const Section *sect) const {
	return std::make_unique<SubStream>(stream.get(), sect->offset, sect->size, false);
}

void SectionedFile::readSectionHeaders() {
	BinaryReader reader(stream.get(), false);
	
	auto mainId = reader.readString(4);
	
	reader.skip(4);
	
	size_t fileSize = reader.readLELong();
	auto headerSize = reader.readLEShort();
	
	assert(headerSize==0x10);
	assert(stream->getLength()>=fileSize);
	
	auto sectionCount = reader.readLEShort();
	
	for(int sectionIndex=0; sectionIndex<sectionCount;++sectionIndex) {
		std::string name = reader.readString(4);
		size_t size = reader.readLELong();
		size_t pos = stream->getPos();
		
		reader.skip(size-8);
		
		sections.emplace_back(name, size, pos);
	}
}

std::vector<std::string> SectionedFile::parseLBAL(int count) const {
	std::vector<std::string> labels;
	
	std::unique_ptr<BinaryReadStream> lablData = getSectionData("LBAL");
	
	if(!lablData) return labels;
	
	labels.reserve(count);
	
	std::vector<size_t> offsets;
	offsets.reserve(count);
	
	BinaryReader reader(std::move(lablData));
	
	for(int i=0;i<count;++i) {
		offsets.emplace_back(reader.readLELong());
	}
	
	size_t basePos = reader.getPos();
	
	for(int i=0;i<count;++i) {
		reader.setPos(offsets[i]+basePos);
		auto str=reader.readZeroTermString();
		labels.push_back(str);
		
	}
	
	return labels;
}