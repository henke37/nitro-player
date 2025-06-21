#ifndef BITITERATOR_H
#define BITITERATOR_H

#include <cstdint>
#include <cstddef>

class BitIterator {
public:
	BitIterator(const uint8_t *buff, size_t buffLen);
	
	bool readBit();
	void seek(size_t bitPos);
	void rewind() { seek(0); }
	
	bool isEOF() const;
private:
	const uint8_t *buff;
	size_t buffLen;
	
	int bitCnt;
	size_t buffPos;
	
	void advance();
	bool currentBit() const;
};

#endif