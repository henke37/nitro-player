#ifndef NINESLICE_H
#define NINESLICE_H

#include "tilePos.h"
#include "nscr.h"

struct NineSliceData {
	TilePos topLeft;
	TilePos topMiddle;
	TilePos topRight;
	TilePos middleLeft;
	TilePos center;
	TilePos middleRight;
	TilePos bottomLeft;
	TilePos bottomMiddle;
	TilePos bottomRight;
	
	const NSCR *screen;
	
	void drawNineSlice(TileMapEntry16 *tilemap, unsigned int xDst, unsigned int yDst, unsigned int drawWidth, unsigned int drawHeight, bool filled=true) const;
};



#endif