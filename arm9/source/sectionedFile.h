#ifndef SECTIONED_FILE_H
#define SECTIONED_FILE_H

#include <string>
#include <vector>
#include <memory>

#include "binaryStream.h"

class SectionedFile {
public:
	SectionedFile(const std::string &filename);
	SectionedFile(std::unique_ptr<BinaryReadStream> &&stream);
	~SectionedFile();
	
	class Section {
	public:
		std::string name;
		size_t size;
		size_t offset;
		
		Section(std::string name, size_t size, size_t offset) : name(name), size(size), offset(offset) {}
	};

	std::unique_ptr<BinaryReadStream> getSectionData(const std::string &sectionName) const;
	std::unique_ptr<BinaryReadStream> getSectionData(size_t sectionOffset) const;
	std::unique_ptr<BinaryReadStream> getSectionData(const SectionedFile::Section *) const;
	
	const SectionedFile::Section *getSectionInfo(const std::string &sectionName) const;
	const SectionedFile::Section *getSectionInfo(size_t sectionOffset) const;
	
	std::vector<std::string> parseLBAL(int count) const;
	
private:
	std::vector<Section> sections;
	
	void readSectionHeaders();
	
	std::unique_ptr<BinaryReadStream> stream;
};

#endif