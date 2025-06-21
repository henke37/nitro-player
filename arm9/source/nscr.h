#ifndef NSCR_H
#define NSCR_H

#include <nds.h>

#include "sectionedFile.h"
#include "charSet.h"
#include "palSet.h"
#include "tilePos.h"

constexpr unsigned int bgWidth=32;
constexpr unsigned int bgHeight=32;

class NSCR {
public:
	NSCR(const std::string &filename,
	const CharSet *chars,
	const PalSet *pals);
	NSCR(std::unique_ptr<BinaryReadStream> &&stream,
	const CharSet *chars,
	const PalSet *pals);
	
	uint16_t getColorMode() const { return colorMode; }
	uint16_t getBgType() const { return bgType; }
	
	void copyRect(void *screenBase, TilePos dst, TilePos src, unsigned int drawWidth, unsigned int drawHeight, int palOffset = 0) const;
	void copyRect(void *screenBase, unsigned int xDst, unsigned int yDst, unsigned int xSrc, unsigned int ySrc, unsigned int drawWidth, unsigned int drawHeight, int palOffset=0) const;
	void copyRect(void *screenBase) const;
	void copyRectToVram(void *screenBase) const;
	void copyRect(void *screenBase, unsigned int xDst, unsigned int yDst, unsigned int xSrc, unsigned int ySrc, unsigned int drawWidth, unsigned int drawHeight, int palOffset, bool flipV, bool flipH) const;
	
	TileMapEntry16 getTile(TilePos pos) const { return getTile(pos.x, pos.y); }
	TileMapEntry16 getTile(unsigned int x, unsigned int y) const;


	TileMapEntry16 getRawTile(TilePos pos) const { return getRawTile(pos.x, pos.y); }
	TileMapEntry16 getRawTile(unsigned int x, unsigned int y) const;


	uint16_t getCols() const { return cols; }
	uint16_t getRows() const { return rows; }
	
private:
	SectionedFile sections;
	
	uint16_t cols;
	uint16_t rows;
	uint16_t colorMode;
	uint16_t bgType;

	void readData();
	
	mutable std::unique_ptr<BinaryReadStream> tileMapStream;
	
	const CharSet *chars;
	const PalSet *pals;
	
	void seekTo(unsigned int x, unsigned int y) const;
	
	void copyRectTxt(void *screenBase, unsigned int xDstBase, unsigned int yDstBase, unsigned int xSrcBase, unsigned int ySrcBase, unsigned int drawWidth, unsigned int drawHeight, int palOffset = 0) const;
	void copyRectTxt(void *screenBase) const;
	
	void copyLineTxt(TileMapEntry16 *mapData, unsigned int xSrcBase, unsigned int ySrc, unsigned int drawWidth, TileMapEntry16 *readBuff, int palOffset = 0) const;
	
	void copyRectAffine(void *screenBase, unsigned int xDstBase, unsigned int yDstBase, unsigned int xSrcBase, unsigned int ySrcBase, unsigned int drawWidth, unsigned int drawHeight, int palOffset = 0) const;
	void copyRectAffine(void *screenBase) const;
	
	unsigned int getScreenWidth(int x) const;
	unsigned int getScreenHeight(int y) const;
	unsigned int getScreenStartX(int x) const;
	unsigned int getScreenStartY(int y) const;
};

enum NNSG2dColorMode {
	NNS_G2D_SCREENCOLORMODE_16x16,
	NNS_G2D_SCREENCOLORMODE_256x1,
	NNS_G2D_SCREENCOLORMODE_256x16
};

enum NNSG2dScreenFormat {
	NNS_G2D_SCREENFORMAT_TEXT,
	NNS_G2D_SCREENFORMAT_AFFINE,
	NNS_G2D_SCREENFORMAT_AFFINEEXT
};

#endif