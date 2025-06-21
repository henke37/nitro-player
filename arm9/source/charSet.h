#ifndef CHARSET_H
#define CHARSET_H

#include "palSet.h"

enum GXOBJVRamModeChar {
	GX_OBJVRAMMODE_CHAR_2D,
	GX_OBJVRAMMODE_CHAR_1D_32K,
	GX_OBJVRAMMODE_CHAR_1D_64K,
	GX_OBJVRAMMODE_CHAR_1D_128K,
	GX_OBJVRAMMODE_CHAR_1D_256K
};

enum NNSG2dCharacterFmt {
	NNS_G2D_CHARACTER_FMT_CHAR,
	NNS_G2D_CHARACTER_FMT_BMP
};

class CharSet {
public:
	virtual int getStartTileIndex() const=0;
	
	virtual uint32_t getFMT() const=0;
	virtual uint16_t getWidth() const=0;
	virtual uint16_t getHeight() const=0;
	virtual uint32_t getVRamMode() const=0;
	virtual bool getTileBased() const=0;

	int getBpp() const {
		return getFMT() == GX_TEXFMT_PLTT256 ? 8 : 4;
	}
};

#endif