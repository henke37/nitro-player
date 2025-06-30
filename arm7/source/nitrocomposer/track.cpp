#include "sequencePlayer.h"

SequencePlayer::Track::Track() : 
	isPlaying(false), 
	noteWait(true), 
	tieMode(false),
	attack(0xFF),
	decay(0xFF),
	sustain(0xFF),
	release(0xFF)
{}

void SequencePlayer::Track::Tick() {
	if(!isPlaying) return;
}