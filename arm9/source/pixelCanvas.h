#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H

#include <cstdint>
#include <cstddef>

#include "charSet.h"
#include "tileMan.h"
#include "palSet.h"
#include "poke.h"

class PixelCanvas : CharSet {
public:
	PixelCanvas(int bpp, unsigned int drawAreaWidth, unsigned int drawAreaHeight);
	~PixelCanvas();
	
	void clear(uint8_t color);
	void setPixel(uint8_t x, uint8_t y, uint8_t color);
	void setPixelClipped(uint8_t x, uint8_t y, uint8_t color);
	void drawHLine(uint8_t startX, uint8_t endX, uint8_t y, uint8_t color);
	void drawVLine(uint8_t x, uint8_t startY, uint8_t endY, uint8_t color);
	
	void reserve(TileManager &);
	void uploadNow(PokeWriteMode mode = PokeWriteMode::DMA_16) const;
	void queueUpload(PokeWriteMode mode = PokeWriteMode::DMA_16) const;
	Poke prepareUpload(PokeWriteMode mode) const;
	
	int getStartTileIndex() const {return reservation.getStartTileIndex();}
	
	uint16_t getWidth() const { return drawAreaWidth; }
	uint16_t getHeight() const { return drawAreaHeight; }
	uint32_t getVRamMode() const;
	uint32_t getFMT() const;
	bool getTileBased() const {return true; }

	
private:
	int bpp;
	unsigned int drawAreaWidth;
	unsigned int drawAreaHeight;
	uint8_t *drawAreaBuffer;
	
	TileReservationToken reservation;
	
	size_t getDrawBuffSize() const;
	size_t posToOffset(uint8_t x, uint8_t y) const;
	
};

#endif