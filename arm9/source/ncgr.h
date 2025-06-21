#ifndef NCGR_H
#define NCGR_H

#include "sectionedFile.h"
#include "nclr.h"
#include "charSet.h"
#include "tileMan.h"
#include "poke.h"

class NCGR : public CharSet {
public:
	NCGR(const std::string &filename);
	NCGR(std::unique_ptr<BinaryReadStream> &&stream);

	uint32_t getFMT() const { return fmt; }
	uint16_t getWidth() const { return w; }
	uint16_t getHeight() const { return h; }
	uint32_t getVRamMode() const { return vramMode; }
	bool getTileBased() const { return tileBased; }
	
	void uploadNow(TileManager &, PokeWriteMode mode = PokeWriteMode::DMA_16);
	void queueUpload(TileManager &, PokeWriteMode mode = PokeWriteMode::DMA_16);
	Poke prepareUpload(TileManager &, PokeWriteMode mode);
	
	int getStartTileIndex() const {return reservation.getStartTileIndex();}
	
private:
	SectionedFile sections;
	
	uint16_t w;
	uint16_t h;
	uint32_t fmt;
	uint32_t vramMode;
	bool tileBased;
	
	void readData();
	
	TileReservationToken reservation;
	
	std::unique_ptr<BinaryReadStream> pixelData;
};

#endif