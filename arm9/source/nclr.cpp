#include "nclr.h"

#include "binaryReader.h"
#include "substream.h"

#include "globals.h"
#include "vblankBatcher.h"

#include <cassert>

NCLR::NCLR(const std::string &filename) : sections(filename) {
	readData();
}
NCLR::NCLR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
	readData();
}

void NCLR::readData() {
	std::unique_ptr<BinaryReadStream> plttData = sections.getSectionData("TTLP");
	
	assert(plttData);
	
	BinaryReader hdrRead{plttData.get(),false};
	
	fmt=hdrRead.readLELong();
	extPal=hdrRead.readLELong()!=0;
	size_t colorLen=hdrRead.readLELong();
	size_t colorOffset=hdrRead.readLELong();
	
	assert(fmt==GX_TEXFMT_PLTT16 || fmt==GX_TEXFMT_PLTT256);
	
	colorData=std::make_unique<SubStream>(std::move(plttData), colorOffset, colorLen);
}

void NCLR::upload256Now(PalMan256 *man, int slot, PokeWriteMode writeMode) {
	prepareUpload256(man, slot, writeMode).Perform();
}
void NCLR::queueUpload256(PalMan256 *man, int slot, PokeWriteMode writeMode) {
	vblankBatcher.AddPoke(prepareUpload256(man, slot, writeMode));
}

Poke NCLR::prepareUpload256(PalMan256 *man, int slot, PokeWriteMode writeMode) {
	assert(fmt == GX_TEXFMT_PLTT256);
	fullPalReserv = man->reserveFullPal(slot);

	auto lock = fullPalReserv.acquireLock();

	uint16_t* palAddr = lock.getDataPtr();

	uint16_t* scratch = new uint16_t[256];
	colorData->setPos(0);
	size_t readCnt = colorData->read((u8*)scratch, fullPalSize);
	assert(readCnt == fullPalSize);

	return Poke(scratch, fullPalSize, palAddr, writeMode);
}

void NCLR::queueUpload(PalMan *man, int pal, PokeWriteMode writeMode) {
	vblankBatcher.AddPoke(prepareUpload(man, pal, writeMode));
}

void NCLR::uploadNow(PalMan *man, int pal, PokeWriteMode writeMode) {
	prepareUpload(man, pal, writeMode).Perform();
}

Poke NCLR::prepareUpload(PalMan *man, int pal, PokeWriteMode writeMode) {
	assert(fmt == GX_TEXFMT_PLTT16);
	assert(pal>=0 && pal<16);
	palReservs[pal]=man->reserve();	
	uint16_t *palAddr=palReservs[pal].getDataPtr();
	
	uint16_t *scratch= new uint16_t[16];
	
	this->readPal(scratch, pal);

	return Poke(scratch, palSize, palAddr, writeMode);
}

void NCLR::uploadNow(PalMan *man, int startPal, int palCnt) {
	assert(fmt == GX_TEXFMT_PLTT16);

	int endPal=startPal+palCnt;
	
	assert(startPal>=0 && startPal<16);
	assert(endPal>=0 && endPal<16);
	
	uint16_t *scratch=new uint16_t[palCnt * 16];
	u8 *srcBuff=(u8*)/*memUncached*/(scratch);
	
	colorData->setPos(palSize*startPal);
	size_t readSize = palSize*palCnt;
	size_t readCnt = colorData->read(srcBuff, readSize);
	
	assert(readCnt==readSize);

	DC_FlushRange(srcBuff, readSize);
	
	for(int pal=startPal, i=0;pal<endPal;++pal,++i) {
		palReservs[pal]=man->reserve();	
		uint16_t *palAddr=palReservs[pal].getDataPtr();
		
		dmaCopyHalfWords(3, srcBuff+(i*palSize), palAddr, palSize);
	}
	
	delete[] scratch;
}

void NCLR::unload() {
	fullPalReserv.clear();
	for(int pal = 0; pal < 16; ++pal) {
		palReservs[pal].clear();
	}
}

void NCLR::unload(int pal) {
	palReservs[pal].clear();
}

void NCLR::unload(int startPal, int palCnt) {
	int endPal=startPal+palCnt;
	for(int pal=startPal, i=0;pal<endPal;++pal,++i) {
		palReservs[pal].clear();
	}
}

void NCLR::readPal(uint16_t *palAddr, int pal) const {
	assert(pal >= 0);
	assert(pal <= 15);
	colorData->setPos(palSize * pal);
	size_t readCnt = colorData->read((u8 *)palAddr, palSize);

	assert(readCnt == palSize);
}


int NCLR::getLoadedPalSlot(unsigned int pal) const {
	if(fullPalReserv.isActive()) {
		assert(fullPalReserv.getMan()->getPalCount()>pal);
		return pal;
	}
	if(!palReservs[pal].isActive()) {
		printf("Pal not loaded %i", pal);
	}
	assert(palReservs[pal].isActive());
	return palReservs[pal].getSlot();
}