#include "sequencePlayer.h"

SequencePlayer::Track::Track() : isPlaying(false) {}

void SequencePlayer::Track::Tick() {
	if(!isPlaying) return;
}