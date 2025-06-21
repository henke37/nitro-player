#ifndef NANR_H
#define NANR_H

#include <memory>
#include <string>
#include <vector>

#include "animationFrame.h"
#include "binaryStream.h"
#include "sectionedFile.h"

class NANR : public AnimationBank {
public:
	NANR(const std::string &filename);
	NANR(std::unique_ptr<BinaryReadStream> &&stream);
	
	
private:
	SectionedFile sections;
	void readData();
	
	void parseABNKData(std::unique_ptr<BinaryReadStream> &&stream);
};

enum NNSG2dAnimationElement {
	NNS_G2D_ANIMELEMENT_INDEX = 0x0,
	NNS_G2D_ANIMELEMENT_INDEX_SRT,
	NNS_G2D_ANIMELEMENT_INDEX_T 
};

#endif