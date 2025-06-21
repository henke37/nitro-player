#include "nftr.h"

#include "binaryReader.h"
#include "substream.h"

#include <cassert>
#include <nds/arm9/sassert.h>

static NFTR::ChrWidth readChrWidth(BinaryReader &);

NFTR::NFTR(const std::string &filename) : sections(filename) {
	readData();
}
NFTR::NFTR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
	readData();
}

NFTR::~NFTR() {}

void NFTR::readData() {
	std::unique_ptr<BinaryReadStream> finfData = sections.getSectionData("FNIF");
	assert(finfData);
	parseFinfData(std::move(finfData));
}

void NFTR::parseFinfData(std::unique_ptr<BinaryReadStream> &&finfData) {
	BinaryReader hdrRead{std::move(finfData)};
	
	uint8_t type = hdrRead.readByte();
	assert(type==0);
	lineFeed = hdrRead.readByte();
	altCharIndex=hdrRead.readLEShort();
	defChrWidth=readChrWidth(hdrRead);
	encoding = hdrRead.readByte();
	
	uint32_t pGlyph = hdrRead.readLELong();
	uint32_t pWidth = hdrRead.readLELong();
	uint32_t pMap = hdrRead.readLELong();
	
	height = hdrRead.readByte();
	width = hdrRead.readByte();
	ascent = hdrRead.readByte();
	//uint8_t padding = hdrRead.readByte();
	
	std::unique_ptr<BinaryReadStream> cglpData = sections.getSectionData(pGlyph);
	assert(cglpData);
	parseCglpData(std::move(cglpData));
	
	while(pWidth) {
		std::unique_ptr<BinaryReadStream> cwdhData = sections.getSectionData(pWidth);
		assert(cwdhData);
		pWidth=parseCwdhData(std::move(cwdhData));
	}
	
	while(pMap) {
		std::unique_ptr<BinaryReadStream> cmapData = sections.getSectionData(pMap);
		assert(cmapData);
		pMap=parseCmapData(std::move(cmapData));
	}
}

void NFTR::parseCglpData(std::unique_ptr<BinaryReadStream> &&cglpData) {
	BinaryReader hdrRead{cglpData.get(),false};
	cellWidth = hdrRead.readByte();
	cellHeight = hdrRead.readByte();
	cellSize = hdrRead.readLEShort();
	baseLinePos = hdrRead.readSignedByte();
	maxCharWidth = hdrRead.readByte();
	bpp = hdrRead.readByte();
	flags = hdrRead.readByte();
	
	pixelData=std::make_unique<SubStream>(std::move(cglpData), cglpData->getPos(), 0xFFFFFFFF);
}

uint32_t NFTR::parseCwdhData(std::unique_ptr<BinaryReadStream> &&cwdhData) {
	BinaryReader hdrRead{std::move(cwdhData)};
	
	uint16_t indexBegin = hdrRead.readLEShort();
	uint16_t indexEnd = hdrRead.readLEShort();
	uint32_t pNext=hdrRead.readLELong();
	
	std::vector<ChrWidth> blockWidths;
	
	blockWidths.reserve(indexEnd-indexBegin+1);
	
	for(uint16_t charIndex=indexBegin;charIndex<indexEnd;++charIndex){
		blockWidths.push_back(readChrWidth(hdrRead));
	}
	
	this->widths.emplace_back(indexBegin,indexEnd, std::move(blockWidths));
	
	return pNext;
}

const NFTR::ChrWidth &NFTR::getGlyphWidth(uint16_t index) const {
	for(auto itr=widths.cbegin();itr<widths.cend();++itr) {
		auto &elm=*itr;
		if(index<elm.indexBegin || index>elm.indexEnd) continue;
		return elm.getWidth(index);
	}
	return defChrWidth;
}

const NFTR::ChrWidth &NFTR::WidthData::getWidth(uint16_t index) const {
	index-=indexBegin;
	return widths[index];
}

unsigned int NFTR::getTextWidth(const std::u16string &text) const {
	unsigned int textWidth=0;
	for(auto itr=text.cbegin();itr!=text.cend();++itr) {
		char16_t c=*itr;
		
		uint16_t index=getGlyphIndex(c);
		const NFTR::ChrWidth &glyphWidth=getGlyphWidth(index);
		
		textWidth+=glyphWidth.charWidth;
	}
	return textWidth;
}


uint32_t NFTR::parseCmapData(std::unique_ptr<BinaryReadStream> &&cmapData) {
	BinaryReader hdrRead{cmapData.get(),false};
	uint16_t ccodeBegin=hdrRead.readLEShort();
	uint16_t ccodeEnd=hdrRead.readLEShort();
	uint16_t mappingMethod=hdrRead.readLEShort();
	hdrRead.skip(2);//padding
	uint32_t pNext=hdrRead.readLELong();
	
	switch((MappingMethod)mappingMethod) {
		case MappingMethod::Direct: {
			uint16_t offset=hdrRead.readLEShort();
			maps.emplace_back(std::make_unique<DirectMappingData>(ccodeBegin, ccodeEnd, offset));
		} break;
		case MappingMethod::Table: {
			maps.emplace_back(std::make_unique<TableMappingData>(ccodeBegin, ccodeEnd, hdrRead));
		} break;
		case MappingMethod::Scan: {
			maps.emplace_back(std::make_unique<ScanMappingData>(ccodeBegin, ccodeEnd, hdrRead));
		} break;
		default:
			sassert(0,"Bad mapping method %hx", mappingMethod);
	}
	
	return pNext;
}

uint16_t NFTR::getGlyphIndex(char16_t c) const {
	for(auto itr=maps.cbegin();itr<maps.cend();++itr) {
		auto &elm=*itr;
		uint16_t index=elm->getGlyphIndex(c);
		if(index!=0xFFFF) return index;
	}
	return altCharIndex;
}

NFTR::MappingData::MappingData(uint16_t ccodeBegin, uint16_t ccodeEnd) : ccodeBegin(ccodeBegin), ccodeEnd(ccodeEnd) {}
NFTR::MappingData::~MappingData() {}

NFTR::DirectMappingData::DirectMappingData(uint16_t ccodeBegin, uint16_t ccodeEnd, uint16_t offset) : MappingData(ccodeBegin, ccodeEnd), offset(offset) {}
NFTR::DirectMappingData::~DirectMappingData() {}

NFTR::TableMappingData::TableMappingData(uint16_t ccodeBegin, uint16_t ccodeEnd,BinaryReader &rdr) : MappingData(ccodeBegin, ccodeEnd) {
	table.reserve(ccodeEnd-ccodeBegin+1);
	
	for(uint16_t ccode=ccodeBegin;ccode<ccodeEnd;++ccode) {
		table.push_back(rdr.readLEShort());
	}
}
NFTR::TableMappingData::~TableMappingData() {}

NFTR::ScanMappingData::ScanMappingData(uint16_t ccodeBegin, uint16_t ccodeEnd,BinaryReader &rdr) : MappingData(ccodeBegin, ccodeEnd) {
	uint16_t numEnts=rdr.readLEShort();
	table.reserve(numEnts);
	
	assert(numEnts<ccodeEnd-ccodeBegin);
	
	for(uint16_t itr=0;itr<numEnts;++itr) {
		uint16_t ccode=rdr.readLEShort();
		uint16_t index=rdr.readLEShort();
		table.emplace_back(ccode, index);
	}
}
NFTR::ScanMappingData::~ScanMappingData() {}

uint16_t NFTR::MappingData::getGlyphIndex(char16_t c) const {
	if(c<ccodeBegin) return 0xFFFF;
	if(c>ccodeEnd) return 0xFFFF;
	return getGlyphIndexImpl(c);
}

NFTR::WidthData::WidthData(uint16_t indexBegin, uint16_t indexEnd, std::vector<ChrWidth> &&widths) :indexBegin(indexBegin), indexEnd(indexEnd), widths(std::move(widths)) {}

uint16_t NFTR::DirectMappingData::getGlyphIndexImpl(char16_t c) const {
	return c+offset-ccodeBegin;
}

uint16_t NFTR::TableMappingData::getGlyphIndexImpl(char16_t c) const {
	return table[c];
}

uint16_t NFTR::ScanMappingData::getGlyphIndexImpl(char16_t c) const {
	//TODO: binary search
	for(auto itr=table.cbegin();itr<table.cend();++itr) {
		auto &elm=*itr;
		if(elm.ccode==c) return elm.index;
	}
	return 0xFFFF;
}

void NFTR::getGlyphPixelData(uint16_t index, uint8_t *outBuff) const {
	pixelData->setPos(index*cellSize);
	pixelData->read(outBuff, cellSize);
}

NFTR::ScanMappingData::MapEntry::MapEntry(uint16_t ccode, uint16_t index) : ccode(ccode), index(index) {}


static NFTR::ChrWidth readChrWidth(BinaryReader &rdr) {
	NFTR::ChrWidth cw;
	
	cw.left = rdr.readSignedByte();
	cw.glyphWidth = rdr.readByte();
	cw.charWidth = rdr.readSignedByte();
	
	return cw;
}