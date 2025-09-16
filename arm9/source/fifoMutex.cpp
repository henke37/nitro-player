#include "fifoMutex.h"

#include <nds/arm9/sassert.h>

constexpr u32 invalidFifoChannel = 0xFFFFFFFF;

FifoMutex::FifoMutex() : channel(invalidFifoChannel) {}

FifoMutex::FifoMutex(u32 channel) : channel(invalidFifoChannel) {
	aquire(channel);
}

FifoMutex::~FifoMutex() {
	if(isAquired()) release();
}

bool FifoMutex::isAquired() const {
	return channel!=invalidFifoChannel;
}

void FifoMutex::aquire(u32 channel) {
	assert(channel != invalidFifoChannel);
	sassert(!isAquired(), "Already aquired %li, can't aquire %li", this->channel, channel);
	fifoMutexAcquire(channel);
	this->channel = channel;
}

bool FifoMutex::tryAquire(u32 channel) {
	assert(channel != invalidFifoChannel);
	sassert(!isAquired(), "Already aquired %li, can't aquire %li", this->channel, channel);
	bool success=fifoMutexTryAcquire(channel);
	if(!success) return false;
	this->channel = channel;
	return true;
}

void FifoMutex::release() {
	sassert(isAquired(), "Can't release without aquire");
	fifoMutexRelease(channel);
	this->channel = invalidFifoChannel;
}
