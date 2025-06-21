#include "bitIterator.h"

#include <cassert>

BitIterator::BitIterator(const uint8_t *buff, size_t buffLen) : buff(buff), buffLen(buffLen), bitCnt(0), buffPos(0) {
	assert(buff);
}

bool BitIterator::isEOF() const {
	return buffPos>=buffLen;
}

void BitIterator::seek(size_t bitPos) {
	bitCnt=bitPos % 8;
	buffPos=bitPos / 8;
}

bool BitIterator::readBit() {
	bool bit=currentBit();
	advance();
	return bit;
}

bool BitIterator::currentBit() const {
	assert(!isEOF());
	uint8_t byte=buff[buffPos];
	return byte & (1<<(7-bitCnt));
}

void BitIterator::advance() {
	if(bitCnt>=7) {
		++buffPos;
		bitCnt=0;
	} else {
		++bitCnt;
	}
}