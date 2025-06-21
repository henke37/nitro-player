#include <nds.h>

#include "animationPlayer.h"
#include <cassert>

AnimAssets::AnimAssets(const AnimationBank *anims,
	const CellBank *cells,
	const CharSet *chars,
	const PalSet *pals,
	OAMManager *oamManager) : anims(anims),
	cells(cells),
	chars(chars),
	pals(pals),
	oamManager(oamManager) {
}


AnimationPlayer::AnimationPlayer(AnimAssets assets) : display(nullptr, assets.chars, assets.pals, assets.oamManager), anims(assets.anims), cells(assets.cells), curAnim(nullptr), curFrameIndex(0), playbackCounter(0), pos(0,0), hFlip(false), vFlip(false), playbackRate(100) {
	assert(anims);
	assert(cells);
}

void AnimationPlayer::clear() {
	setAnim(nullptr);
}

void AnimationPlayer::setAnim(const std::string &name) {
	const Animation *anim=&anims->getAnim(name);
	setAnim(anim);
}

void AnimationPlayer::setAnim(size_t index) {
	const Animation *anim=&anims->getAnim(index);
	setAnim(anim);
}

void AnimationPlayer::setAnim(const Animation *anim) {
	if(anim==this->curAnim) return;

	playbackRate = 100;
	
	this->curAnim=anim;
	curFrameIndex=0;
	playbackCounter=0;
	setupCurrentFrame();
}

void AnimationPlayer::update() {
	if(!curAnim) return;
	
	playbackCounter-=playbackRate;
	
	while(playbackCounter<=0) {
		playbackCounter+=100;
		updateFrame();
	}
}

void AnimationPlayer::updateFrame() {
	curFrameDisplayCounter--;
	
	if(curFrameDisplayCounter>0) {
		return;
	}
	
	selectNextFrame();
	setupCurrentFrame();
}

void AnimationPlayer::selectNextFrame() {
	switch(curAnim->playbackMode) {
		case Forward:
			if((curFrameIndex+1) >= curAnim->frames.size()) {
				stopPlayback();
				return;
			} else {
				++curFrameIndex;
			}
		break;
		case ForwardLoop:
			if((curFrameIndex+1) >= curAnim->frames.size()) {
				curFrameIndex = curAnim->loopStart;
				return;
			} else {
				++curFrameIndex;
			}
		break;
		case PingPongOnce:
		case PingPongLoop:
		default:
			sassert(0,"Bad playback mode %i",(int)curAnim->playbackMode);
		break;
	}
}

void AnimationPlayer::stopPlayback() {
	playbackRate=0;
}

void AnimationPlayer::setupCurrentFrame() {
	if(!curAnim) return;

	const AnimationFrame &curFrame = curAnim->frames.at(curFrameIndex);
	
	curFrameDisplayCounter = curFrame.displayTime;
	
	display.cell = &cells->getCell(curFrame.cellIndex);
	setDisplayProps();
}

void AnimationPlayer::setDisplayProps() {
	if(!curAnim) return;
	const AnimationFrame &curFrame = curAnim->frames.at(curFrameIndex);
	
	display.displaySettings.xOff = pos.x + curFrame.xOff;
	display.displaySettings.yOff = pos.y + curFrame.yOff;
	display.displaySettings.hFlip = hFlip;
	display.displaySettings.vFlip = vFlip;
}

void AnimationPlayer::setPos(Point newPos) {
	pos = newPos;
	setDisplayProps();
}

void AnimationPlayer::setFlip(bool hFlip, bool vFlip) {
	this->hFlip=hFlip;
	this->vFlip=vFlip;
	setDisplayProps();
}

void AnimationPlayer::setPosAndFlip(Point newPos, bool hFlip, bool vFlip) {
	pos = newPos;
	this->hFlip=hFlip;
	this->vFlip=vFlip;
	setDisplayProps();
}