#include "pixelCanvas.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include "globals.h"
#include "vblankBatcher.h"

#include <nds.h>

PixelCanvas::PixelCanvas(int bpp, unsigned int drawAreaWidth, unsigned int drawAreaHeight) : bpp(bpp), drawAreaWidth(drawAreaWidth), drawAreaHeight(drawAreaHeight) {
	assert(bpp==8 || bpp==4);
	assert(drawAreaWidth>0 && drawAreaWidth<=256);
	assert(drawAreaHeight>0 && drawAreaHeight<=192);
	assert((drawAreaWidth%8)==0);
	assert((drawAreaHeight%8)==0);
	
	drawAreaBuffer=new uint8_t[getDrawBuffSize()];
}

PixelCanvas::~PixelCanvas() {
	delete[] drawAreaBuffer;
}

void PixelCanvas::clear(uint8_t color) {
	uint8_t fillVal=(bpp==4)?(color|(color<<4)):color;
	memset(drawAreaBuffer, fillVal, getDrawBuffSize());
}

size_t PixelCanvas::getDrawBuffSize() const {
	if(bpp==4) {
		return drawAreaWidth*drawAreaHeight/2;
	} else {
		return drawAreaWidth*drawAreaHeight;
	}
}

size_t PixelCanvas::posToOffset(uint8_t x, uint8_t y) const {
	assert(x<drawAreaWidth);
	assert(y<drawAreaHeight);
	
	uint8_t tileX=x/8;
	uint8_t tileY=y/8;
	
	uint8_t localX=x%8;
	uint8_t localY=y%8;
	
	size_t tileOffset=(tileX+tileY*(drawAreaWidth/8))*8*8;
	size_t offset=localX+localY*8+tileOffset;
	
	if(bpp==4) {
		assert((x%2)==0);
		return offset/2;
	} else {
		return offset;
	}
}

void PixelCanvas::setPixelClipped(uint8_t x, uint8_t y, uint8_t newColor) {
	if(x>=drawAreaWidth) return;
	if(y>=drawAreaHeight) return;
	setPixel(x, y, newColor);
}

void PixelCanvas::setPixel(uint8_t x, uint8_t y, uint8_t newColor) {
	if(bpp==4) {
		assert(newColor<16);
		bool right=x%2;
		x &= ~1;
		size_t offset=posToOffset(x, y);
		
		uint8_t col=drawAreaBuffer[offset];
		if(right) {
			col=(col&0x0F) | (newColor << 4);
		} else {
			col=(col&0xF0) | newColor;
		}
		drawAreaBuffer[offset]=col;
	} else {
		size_t offset=posToOffset(x, y);
		drawAreaBuffer[offset]=newColor;
	}
}


void PixelCanvas::drawHLine(uint8_t startX, uint8_t endX, uint8_t y, uint8_t color) {
	for(uint8_t x=startX;x<endX;++x) {
		setPixel(x,y,color);
	}
}
void PixelCanvas::drawVLine(uint8_t x, uint8_t startY, uint8_t endY, uint8_t color) {
	for(uint8_t y=startY;y<endY;++y) {
		setPixel(x,y,color);
	}
}

uint32_t PixelCanvas::getFMT() const {
	return (bpp==4)?GX_TEXFMT_PLTT16:GX_TEXFMT_PLTT256;
}

uint32_t PixelCanvas::getVRamMode() const {
	return GX_OBJVRAMMODE_CHAR_1D_32K;
}

void PixelCanvas::reserve(TileManager &tileMan) {
	size_t tileCount=(drawAreaWidth/8)*(drawAreaHeight/8);
	reservation=std::move(tileMan.reserve(tileCount));
}

void PixelCanvas::uploadNow(PokeWriteMode mode) const {
	prepareUpload(mode).Perform();
}

void PixelCanvas::queueUpload(PokeWriteMode mode) const {
	vblankBatcher.AddPoke(prepareUpload(mode));
}

Poke PixelCanvas::prepareUpload(PokeWriteMode mode) const {
	return Poke(drawAreaBuffer, getDrawBuffSize(), reservation.getDataPtr(), mode);
}