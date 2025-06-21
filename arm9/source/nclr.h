#ifndef NCLR_H
#define NCLR_H

#include "sectionedFile.h"
#include "palMan.h"
#include "palSet.h"
#include "poke.h"

class NCLR : public PalSet {
public:
	NCLR(const std::string &filename);
	NCLR(std::unique_ptr<BinaryReadStream> &&stream);
	
	void upload256Now(PalMan256 *, int slot, PokeWriteMode writeMode = PokeWriteMode::DMA_16);
	void queueUpload256(PalMan256 *, int slot, PokeWriteMode writeMode = PokeWriteMode::DMA_16);
	Poke prepareUpload256(PalMan256 *, int slot, PokeWriteMode writeMode);

	void uploadNow(PalMan *, int pal, PokeWriteMode writeMode = PokeWriteMode::DMA_16);
	void queueUpload(PalMan *, int pal, PokeWriteMode writeMode = PokeWriteMode::DMA_16);
	Poke prepareUpload(PalMan *, int pal, PokeWriteMode writeMode);
	void uploadNow(PalMan *, int startPal, int palCnt);
	//void queueUpload(PalMan *, int startPal, int palCnt);
	//Poke prepareUpload(PalMan *, int startPal, int palCnt);
	void unload();
	void unload(int pal);
	void unload(int startPal, int palCnt);

	void readPal(uint16_t *palAddr, int pal) const override;
	
	int getLoadedPalSlot(unsigned int pal) const override;
	
	uint32_t getFMT() const override { return fmt; }
	bool getExtPal() const override { return extPal; }

private:
	SectionedFile sections;
	PalReservation palReservs[16];
	FullPalReservation fullPalReserv;
	
	uint32_t fmt;
	bool extPal;
	
	std::unique_ptr<BinaryReadStream> colorData;
	
	void readData();
};

#endif