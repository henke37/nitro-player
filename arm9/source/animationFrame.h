#ifndef ANIMATION_H
#define ANIMATION_H

#include "animationCell.h"

#include <vector>

class AnimationFrame {
public:
	int cellIndex;
	int displayTime;
	int xOff;
	int yOff;
	
	AnimationFrame(int cellIndex, int displayTime);
};

class Animation {
public:
	std::vector<AnimationFrame> frames;
	uint32_t playbackMode;
	uint16_t loopStart;
	
	Animation(uint32_t playbackMode, uint16_t loopStart);
};

class AnimationBank {
public:
	const Animation &getAnim(int index) const;
	const Animation &getAnim(const std::string &name) const;
protected:
	std::vector<Animation> anims;
	std::vector<std::string> labels;
};

enum PlaybackMode {
	Invalid = 0,
	Forward = 1,
	ForwardLoop = 2,
	PingPongOnce = 3,
	PingPongLoop = 4
};

#endif