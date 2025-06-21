#include "ncgr.h"

#include <nds.h>

#include <cassert>

#include "binaryReader.h"
#include "substream.h"
#include "globals.h"
#include "vblankBatcher.h"

NCGR::NCGR(const std::string &filename) : sections(filename) {
	readData();
}
NCGR::NCGR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
	readData();
}

void NCGR::readData() {
	std::unique_ptr<BinaryReadStream> charData = sections.getSectionData("RAHC");
	
	assert(charData);
	
	BinaryReader hdrRead{charData.get(),false};
	w=hdrRead.readLEShort();
	h=hdrRead.readLEShort();
	fmt=hdrRead.readLELong();
	vramMode=hdrRead.readLELong();
	tileBased=hdrRead.readLELong()==0;
	
	size_t pixelsLen=hdrRead.readLELong();
	size_t pixelsOffset=hdrRead.readLELong();
	
	pixelData=std::make_unique<SubStream>(std::move(charData), pixelsOffset, pixelsLen);
}


void NCGR::uploadNow(TileManager &tileMan, PokeWriteMode mode) {
	prepareUpload(tileMan, mode).Perform();
}

void NCGR::queueUpload(TileManager &tileMan, PokeWriteMode mode) {
	vblankBatcher.AddPoke(prepareUpload(tileMan, mode));
}

Poke NCGR::prepareUpload(TileManager &tileMan, PokeWriteMode mode) {
	assert(tileMan.getBpp() == this->getBpp());
	size_t tileSize = 8*getBpp();
	int tileCount = pixelData->getLength()/tileSize;
	reservation=tileMan.reserve(tileCount);
	
	std::unique_ptr<uint8_t[]> srcBuff =std::make_unique<uint8_t[]>(pixelData->getLength());
	
	size_t readCnt = pixelData->read(srcBuff.get(), pixelData->getLength());
	
	assert(readCnt==pixelData->getLength());

	return Poke(std::move(srcBuff), pixelData->getLength(), reservation.getDataPtr(), mode);
}