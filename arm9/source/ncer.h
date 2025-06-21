#ifndef NCER_H
#define NCER_H

#include "sectionedFile.h"
#include "animationCell.h"

class NCER : public CellBank {
public:
	NCER(const std::string &filename);
	NCER(std::unique_ptr<BinaryReadStream> &&stream);
	
	
private:
	SectionedFile sections;
	void readData();
	
	void parseCEBKData(std::unique_ptr<BinaryReadStream> &&stream);
};

#endif