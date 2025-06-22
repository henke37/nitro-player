#ifndef NITROCOMPOSER_SBNK_H
#define NITROCOMPOSER_SBNK_H

#include "../sectionedFile.h"

class SBNK {
public:
	SBNK(const std::string &fileName);
	SBNK(std::unique_ptr<BinaryReadStream> &&stream);
private:
	SectionedFile sections;
};

#endif