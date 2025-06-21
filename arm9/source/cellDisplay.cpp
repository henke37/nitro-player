#include "cellDisplay.h"

#include <cassert>

int objW[4][3]= { 8, 16, 8, 16, 32, 8, 32, 32, 16, 64, 64, 32};
int objH[4][3]= { 8, 8, 16, 16, 8, 32, 32, 16, 32, 64, 32, 64};

CellDisplay::CellDisplay(const AnimationCell *cell, const CharSet *chars, const PalSet *pals, OAMManager *oamManager) : OAMWriter(oamManager), cell(cell), chars(chars), pals(pals), displaySettings() {
	assert(chars);
	assert(pals);
}

CellDisplay::DisplaySettings::DisplaySettings() : xOff(0), yOff(0), palOff(0), visible(true), hFlip(false), vFlip(false), rotIndexOff(0), blendMode(ObjBlendMode::OBJMODE_NORMAL) {}

CellDisplay::~CellDisplay() {}

bool CellDisplay::objInBounds(const SpriteEntry &obj) {
	int dbl = (obj.isRotateScale && obj.isSizeDouble) ? 2 : 1;
	int w=objW[obj.size][obj.shape]*dbl;
	int h=objH[obj.size][obj.shape]*dbl;
	
	if(obj.x + w < 0) return false;
	if(obj.x > SCREEN_WIDTH) return false;
	if(obj.y + h < 0) return false;
	if(obj.y > SCREEN_HEIGHT) return false;
	return true;
}

int CellDisplay::getObjCount() const {
	if(!cell) return 0;
	if(!displaySettings.visible) return 0;
	return cell->objectDefs.size();
}

void CellDisplay::drawOam(SpriteEntry **outP) const {
	if(!cell) return;
	if(!displaySettings.visible) return;
	
	SpriteEntry *out=*outP;
	
	for(auto objItr = cell->objectDefs.cbegin(); objItr < cell->objectDefs.cend(); ++objItr) {
		*out = *objItr;
		
		out->x += displaySettings.xOff;
		out->y += displaySettings.yOff;
		
		if(!objInBounds(*out)) continue;
		
		out->palette = pals->getLoadedPalSlot(out->palette + displaySettings.palOff);
		out->gfxIndex += chars->getStartTileIndex();
		
		if(out->isRotateScale) {
			out->rotationIndex += displaySettings.rotIndexOff;
		} else {
			out->hFlip ^= displaySettings.hFlip;
			out->vFlip ^= displaySettings.vFlip;
		}
		out->priority = displaySettings.objPriority;
		out->blendMode = displaySettings.blendMode;
		
		++out;
	}
	
	*outP=out;
}