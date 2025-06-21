#ifndef PALSET_H
#define PALSET_H

constexpr int palSize = 16 * 2;
constexpr int fullPalSize = 256 * 2;

class PalSet {
public:
	virtual int getLoadedPalSlot(unsigned int pal) const=0;
	
	virtual uint32_t getFMT() const =0;
	virtual bool getExtPal() const =0;

	virtual void readPal(uint16_t *palAddr, int pal) const=0;
};

enum GXTexFmt {
	GX_TEXFMT_NONE = 0,
	GX_TEXFMT_A3I5 = 1,
	GX_TEXFMT_PLTT4 = 2,
	GX_TEXFMT_PLTT16 = 3,
	GX_TEXFMT_PLTT256 = 4,
	GX_TEXFMT_COMP4x4 = 5,
	GX_TEXFMT_A5I3 = 6,
	GX_TEXFMT_DIRECT = 7
};

#endif