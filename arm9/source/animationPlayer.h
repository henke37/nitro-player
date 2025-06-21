#include <nds.h>

#ifndef ANIMATION_PLAYER_H
#define ANIMATION_PLAYER_H

#include "point.h"

#include "cellDisplay.h"
#include "animationFrame.h"

struct AnimAssets {
	const AnimationBank *anims;
	const CellBank *cells;
	const CharSet *chars;
	const PalSet *pals;
	OAMManager *oamManager;

	AnimAssets() = default;
	
	AnimAssets(const AnimationBank *anims,
	const CellBank *cells,
	const CharSet *chars,
	const PalSet *pals,
	OAMManager *oamManager);
};

class AnimationPlayer {
	CellDisplay display;
	
	const AnimationBank *anims;
	const CellBank *cells;
	
	const Animation *curAnim;
	
	unsigned int curFrameIndex;
	unsigned int curFrameDisplayCounter;
	
	signed int playbackCounter;
	
	void setAnim(const Animation *);
	
	void updateFrame();
	void selectNextFrame();
	void setupCurrentFrame();
	void setDisplayProps();
	
	Point pos;
	bool hFlip, vFlip;
	
	void stopPlayback();
	
public:
	AnimationPlayer(AnimAssets assets);

	void clear();
	
	void setAnim(const std::string &name);
	void setAnim(size_t index);
	
	void setPos(Point pos);
	void setFlip(bool hFlip, bool vFlip);
	void setPosAndFlip(Point pos, bool hFlip, bool vFlip);
	
	void setPriority(ObjPriority priority) { display.displaySettings.objPriority=priority; }
	void setBlendMode(ObjBlendMode mode) { display.displaySettings.blendMode = mode; }
	void setDrawOrder(int order) { display.drawOrder=order; }
	void setPalOffset(int8_t offset) { display.displaySettings.palOff=offset; }
	void setVisible(bool v) { display.displaySettings.visible = v; }
	
	ObjPriority getPriority() const { return display.displaySettings.objPriority; }
	ObjBlendMode getBlendMode() const { return display.displaySettings.blendMode; }
	int getDrawOrder() const { return display.drawOrder; }
	int8_t getPalOffset() const { return display.displaySettings.palOff; }
	bool getVisible() const { return display.displaySettings.visible; }

	unsigned int getFrameNumber() const {return curFrameIndex; }
	
	unsigned int playbackRate;
	
	void update();
};

#endif