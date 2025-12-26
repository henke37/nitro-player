#include "nscr.h"

#include "binaryReader.h"
#include "substream.h"

#include <nds.h>

#include <cassert>

static_assert(sizeof(TileMapEntry16)==2, "sizeof(TileMapEntry16) must be 2");

NSCR::NSCR(const std::string &filename, const CharSet *chars, const PalSet *pals) : sections(filename), chars(chars), pals(pals) {
	assert(chars);
	assert(pals);
	
	readData();
}
NSCR::NSCR(std::unique_ptr<BinaryReadStream> &&stream, const CharSet *chars, const PalSet *pals) : sections(std::move(stream)), chars(chars), pals(pals) {
	assert(chars);
	assert(pals);
	
	readData();
}

void NSCR::readData() {
	std::unique_ptr<BinaryReadStream> scrnData = sections.getSectionData("NRCS");
	
	assert(scrnData);
	
	BinaryReader hdrRead{scrnData.get(),false};
	cols=hdrRead.readLEShort()/8;
	rows=hdrRead.readLEShort()/8;
	colorMode=hdrRead.readLEShort();
	bgType=hdrRead.readLEShort();
	
	size_t tileMapLen=hdrRead.readLELong();
	size_t tileMapOffset=scrnData->getPos();
	
	tileMapStream=std::make_unique<SubStream>(std::move(scrnData), tileMapOffset, tileMapLen);
}

void NSCR::seekTo(unsigned int x, unsigned int y) const {
	
	int mapId=0;
	
	size_t offset=0;
	
	unsigned int globalW = (cols>bgWidth?bgWidth:cols);
	unsigned int globalH = (rows>bgHeight?bgHeight:rows);
	
	unsigned int bgSize= globalW*globalH;
	
	unsigned int maxXScreens=cols/bgWidth;
	unsigned int thisXScreen=x/bgWidth;
	bool lastXScreen=thisXScreen==maxXScreens;

	unsigned int maxYScreens=rows/bgHeight;
	unsigned int thisYScreen=y/bgHeight;
	bool lastYScreen=thisYScreen==maxYScreens;
	
	unsigned int localW=(lastXScreen?cols%bgWidth:bgWidth);
	unsigned int localH=(lastYScreen?rows%bgHeight:bgHeight);
	
	while(y>=bgHeight) {
		y-=bgHeight;
		mapId+=((cols>bgWidth)?2:1);
	}
	
	while(x>=bgWidth) {
		x-=bgWidth;
		mapId++;
	}
	
	offset=mapId*bgSize;
	
	offset += ((y%localH)*localW+x);
	
	if(bgType != NNS_G2D_SCREENFORMAT_AFFINE) {
		offset *= sizeof(TileMapEntry16);
	}
	
	tileMapStream->setPos(offset);
}

unsigned int NSCR::getScreenWidth(int x) const {
	int screenX=x/bgWidth;
	int screensX=cols/bgWidth;
	
	if(screenX<screensX) return bgWidth;
	return cols%bgWidth;
}

unsigned int NSCR::getScreenHeight(int y) const {
	int screenY=y/bgHeight;
	int screensY=rows/bgHeight;
	
	if(screenY<screensY) return bgHeight;
	return rows%bgHeight;
}

unsigned int NSCR::getScreenStartX(int x) const {
	return (x / bgWidth) * bgWidth;
}
unsigned int NSCR::getScreenStartY(int y) const {
	return (y / bgHeight) * bgHeight;
}

TileMapEntry16 NSCR::getTile(unsigned int x, unsigned int y) const {
	TileMapEntry16 tile = getRawTile(x, y);

	tile.palette = pals->getLoadedPalSlot(tile.palette);
	tile.index += chars->getStartTileIndex();
	
	return tile;
}

TileMapEntry16 NSCR::getRawTile(unsigned int x, unsigned int y) const {
	union {
		TileMapEntry16 tile;
		u8 buff[2];
	} readBuff;

	if(x >= cols) printf("X %i ", x);
	assert(x < cols);
	assert(y < rows);

	seekTo(x, y);
	size_t readLen = sizeof(TileMapEntry16);
	size_t readCnt = tileMapStream->read(readBuff.buff, readLen);
	assert(readCnt == readLen);

	return readBuff.tile;
}

void NSCR::copyRect(void *screenBase) const {
	
	if(bgType==NNS_G2D_SCREENFORMAT_AFFINE) {
		copyRectAffine(screenBase);
	} else {
		copyRectTxt(screenBase);
	}
}

void NSCR::copyRectToVram(void* screenBase) const {
	if(bgType == NNS_G2D_SCREENFORMAT_AFFINE) {
		assert(0);
	} else {
		TileMapEntry16* writeBuff = new TileMapEntry16[cols * rows];
		copyRect(writeBuff, 0, 0, 0, 0, cols, rows);

		size_t buffSize = sizeof(TileMapEntry16) * cols * rows;

		DC_FlushRange(writeBuff, buffSize);
		dmaCopyHalfWords(3, writeBuff, screenBase, buffSize);

		delete[] writeBuff;
	}
}

void NSCR::copyRect(void *screenBase, TilePos dst, TilePos src, unsigned int drawWidth, unsigned int drawHeight, int palOffset) const {
	copyRect(screenBase, dst.x, dst.y, src.x, src.y, drawWidth, drawHeight, palOffset);
}

void NSCR::copyRect(void *screenBase, unsigned int xDst, unsigned int yDst, unsigned int xSrc, unsigned int ySrc, unsigned int drawWidth, unsigned int drawHeight, int palOffset) const {
	if(bgType==NNS_G2D_SCREENFORMAT_AFFINE) {
		copyRectAffine(screenBase, xDst, yDst, xSrc, ySrc, drawWidth, drawHeight, palOffset);
	} else {
		copyRectTxt(screenBase, xDst, yDst, xSrc, ySrc, drawWidth, drawHeight, palOffset);
	}
}

void NSCR::copyRectTxt(void *screenBase) const {
	copyRectTxt(screenBase, 0, 0, 0, 0, cols, rows);
}

void NSCR::copyRectAffine(void *screenBase) const {
	copyRectAffine(screenBase, 0, 0, 0, 0, cols, rows);
}

void NSCR::copyRectTxt(void *screenBase, unsigned int xDstBase, unsigned int yDstBase, unsigned int xSrcBase, unsigned int ySrcBase, unsigned int drawWidth, unsigned int drawHeight, int palOffset) const {
	TileMapEntry16 *mapDataBase=(TileMapEntry16*) screenBase;
	
	TileMapEntry16 *readBuff=new TileMapEntry16[drawWidth];
	
	assert(drawWidth+xDstBase<=bgWidth);
	assert(drawWidth+xSrcBase<=cols);
	assert(drawHeight+ySrcBase<=rows);
	
	for(unsigned int yItr=0;yItr<drawHeight;++yItr) {
		unsigned int yDst=yDstBase+yItr;
		unsigned int ySrc=ySrcBase+yItr;
		
		for(unsigned int xItr=0;xItr<drawWidth;) {
			unsigned int xSrc=xSrcBase+xItr;
			unsigned int xDst=xDstBase+xItr;
			
			unsigned int screenWidth=getScreenWidth(xSrc);
			//unsigned int screenLeft = xSrc % screenWidth;
			unsigned int screenX = getScreenStartX(xSrc);
			unsigned int screenRight = screenX + screenWidth;
			unsigned int remainingToDraw = drawWidth - xItr;
			unsigned int remainingInScreen = screenRight - xSrc;
			
			unsigned int lineLen = remainingInScreen;
			
			if(lineLen > remainingToDraw) {
				lineLen=remainingToDraw;
			}

			TileMapEntry16* mapDst = mapDataBase + (yDst * bgWidth + xDst);
		
			copyLineTxt(mapDst, xSrc, ySrc, lineLen, readBuff, palOffset);
			
			xItr+=lineLen;
			
			assert(xItr<=drawWidth);
		}
	}
	
	delete[] readBuff;
}

void NSCR::copyLineTxt(TileMapEntry16 *mapData, unsigned int xSrcBase, unsigned int ySrc, unsigned int drawWidth, TileMapEntry16 *readBuff, int palOffset) const {
	
	assert(drawWidth>0);
	assert(drawWidth<=bgWidth);
	
	seekTo(xSrcBase, ySrc);
	size_t readLen = drawWidth*sizeof(TileMapEntry16);
	size_t readCnt=tileMapStream->read((uint8_t *)readBuff, readLen);
	assert(readCnt==readLen);
	
	for(unsigned int x=0;x<drawWidth;++x) {
		TileMapEntry16 *mapDstData= mapData +x;
		TileMapEntry16 *mapSrcData=readBuff+x;
		
		TileMapEntry16 entry = *mapSrcData;
		
		entry.palette = pals->getLoadedPalSlot(entry.palette+palOffset);
		entry.index += chars->getStartTileIndex();
		
		*mapDstData = entry;
	}
}

void NSCR::copyRectAffine([[maybe_unused]] void *screenBase, [[maybe_unused]] unsigned int xDstBase, [[maybe_unused]] unsigned int yDstBase, [[maybe_unused]] unsigned int xSrcBase, [[maybe_unused]] unsigned int ySrcBase, [[maybe_unused]] unsigned int drawWidth, [[maybe_unused]] unsigned int drawHeight, [[maybe_unused]] int palOffset) const {
	assert(0);
}