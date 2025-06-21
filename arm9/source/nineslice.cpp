#include "nineslice.h"

void NineSliceData::drawNineSlice(TileMapEntry16 *tilemap, unsigned int xDst, unsigned int yDst, unsigned int drawWidth, unsigned int drawHeight, bool filled) const {
	assert(xDst + drawWidth <= 32);
	assert(yDst + drawHeight <= 24);

	--drawWidth;
	--drawHeight;

	tilemap[xDst+32*yDst]=screen->getTile(topLeft);
	tilemap[xDst+drawWidth+32*yDst]=screen->getTile(topRight);
	
	tilemap[xDst+32*(yDst+drawHeight)]=screen->getTile(bottomLeft);
	tilemap[xDst+drawWidth+32*(yDst+drawHeight)]=screen->getTile(bottomRight);
	
	for(unsigned int x=1;x<drawWidth;++x) {
		tilemap[xDst+x+32*yDst]=screen->getTile(topMiddle);
		tilemap[xDst+x+32*(yDst+drawHeight)]=screen->getTile(bottomMiddle);
	}
	
	for(unsigned int y=1;y<drawHeight;++y) {
		tilemap[xDst+32*(yDst+y)]=screen->getTile(middleLeft);
		tilemap[xDst+drawWidth+32*(yDst+y)]=screen->getTile(middleRight);
	}
	
	if(filled) {
		for(unsigned int y = 1; y < drawHeight; ++y) {
			for(unsigned int x = 1; x < drawWidth; ++x) {
				tilemap[xDst + x + 32 * (yDst + y)] = screen->getTile(center);
			}
		}
	}
}
