#ifndef CELL_DISPLAY_H
#define CELL_DISPLAY_H

#include "animationCell.h"
#include "charSet.h"
#include "palSet.h"
#include "oamMan.h"

class CellDisplay : public OAMWriter {
public:
	CellDisplay(const AnimationCell *cell, const CharSet *chars, const PalSet *pals, OAMManager *oamManager);
	CellDisplay(const CellDisplay &) = default;
	~CellDisplay();

	const AnimationCell *cell;
	const CharSet *chars;
	const PalSet *pals;
	
	struct DisplaySettings {
		int16_t xOff, yOff;
		int8_t palOff;
		bool visible;
		bool hFlip, vFlip;
		uint8_t rotIndexOff;
		ObjPriority objPriority;
		ObjBlendMode blendMode;
		
		DisplaySettings();
	};

	DisplaySettings displaySettings;
	
	int getObjCount() const;
	
	void drawOam(SpriteEntry **out) const;
private:
	static bool objInBounds(const SpriteEntry &obj);
};

#endif