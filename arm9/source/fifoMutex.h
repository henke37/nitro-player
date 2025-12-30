#ifndef FIFOMUTEX_H
#define FIFOMUTEX_H

#include <nds/fifocommon.h>

class FifoMutex {
public:
	FifoMutex();
	FifoMutex(const FifoMutex &) = delete;
	FifoMutex(u32 channel);
	~FifoMutex();

	bool isAquired() const;

	void aquire(u32 channel);
	bool tryAquire(u32 channel);
	void release();
private:
	u32 channel;
};

#endif