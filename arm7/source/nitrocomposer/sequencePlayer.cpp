#include "sequencePlayer.h"

#include <nds/fifocommon.h>
#include <nds/timers.h>
#include <nds/interrupts.h>

SequencePlayer sequencePlayer;

const size_t fifoBuffSize=32;

void SequencePlayer::Init() {
	setupFifo();
	setupTimer();
}

void SequencePlayer::setupFifo() {
	fifoSetDatamsgHandler(FIFO_USER_01, fifoDatagramHandler, nullptr);
}

void SequencePlayer::fifoDatagramHandler(int num_bytes, void *userdata) {
	sequencePlayer.fifoDatagramHandler(num_bytes);
}

void SequencePlayer::fifoDatagramHandler(int num_bytes) {
	u8 fifoBuffer[fifoBuffSize];
	fifoGetDatamsg(FIFO_USER_01, fifoBuffSize, fifoBuffer);
}

void SequencePlayer::setupTimer() {
	timerStart(1, ClockDivider_64, 2728, ISR);
}

void SequencePlayer::ISR() {
	sequencePlayer.Tick();
}

void SequencePlayer::Tick() {

	for(unsigned int trackIndex = 0; trackIndex < 16; ++trackIndex) {
		auto &track = tracks[trackIndex];
		track.Tick();
	}

	for(unsigned int voiceIndex = 0; voiceIndex < 16; ++voiceIndex) {
		auto &voice = voices[voiceIndex];
		voice.Tick();
	}
}


