#include "animationFrame.h"

#include <cassert>

#include <stdio.h>

const Animation &AnimationBank::getAnim(int index) const { return anims.at(index); }
const Animation &AnimationBank::getAnim(const std::string &name) const {
	for(size_t i=0;i<labels.size();++i) {
		if(labels[i]==name) {
			return getAnim(i);
		}
	}
	sassert(0,"Unknown anim \"%s\"!",name.c_str());
}

Animation::Animation(uint32_t playbackMode, uint16_t loopStart) : playbackMode(playbackMode), loopStart(loopStart) {}

AnimationFrame::AnimationFrame(int cellIndex, int displayTime) : cellIndex(cellIndex), displayTime(displayTime), xOff(0), yOff(0) {}