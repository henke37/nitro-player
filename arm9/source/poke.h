#ifndef POKE_H
#define POKE_H

#include <memory>
#include <cstdint>
#include "bitFieldPoke.h"

typedef volatile void *hwPtr;

enum class PokeType {
	NOOP       = 0,
	RAWBLOB_8  = 1,
	RAWBLOB_16 = 2,
	RAWBLOB_32 = 3,
	OWNBLOB_8  = 5,
	OWNBLOB_16 = 6,
	OWNBLOB_32 = 7,
	INT = 8,
	BITFIELD = 9
};

enum class PokeWriteMode {
	INVALID = 0,
	MEMCPY_8 = 1,
	MEMCPY_16 = 2,
	MEMCPY_32 = 3,
	DMA_16 = 4,
	DMA_32 = 5
};

class Poke {
	struct {
		size_t size : 23;
		PokeWriteMode writeMode : 4;
		PokeType type : 5;
	};
	hwPtr addr;
	union {
		uint8_t value8;
		uint16_t value16;
		uint32_t value32;
		std::unique_ptr<const uint8_t[]> valuePtr8;
		std::unique_ptr<const uint16_t[]> valuePtr16;
		std::unique_ptr<const uint32_t[]> valuePtr32;
		const uint8_t *rawPtr8;
		const uint16_t *rawPtr16;
		const uint32_t *rawPtr32;
		BitFieldPoke<uint8_t> bitField8;
		BitFieldPoke<uint16_t> bitField16;
		BitFieldPoke<uint32_t> bitField32;
	};
	
public:
	Poke();
	Poke(Poke &&);
	Poke(uint8_t val, volatile uint8_t *addr);
	Poke(uint16_t val, volatile uint16_t *addr);
	Poke(uint32_t val, volatile uint32_t *addr);
	Poke(std::unique_ptr<const uint8_t[]> &&srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	Poke(std::unique_ptr<const uint16_t[]> &&srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	Poke(std::unique_ptr<const uint32_t[]> &&srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	Poke(const uint8_t* srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	Poke(const uint16_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	Poke(const uint32_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	Poke(uint8_t val, uint8_t mask, volatile uint8_t *addr);
	Poke(uint16_t val, uint16_t mask, volatile uint16_t *addr);
	Poke(uint32_t val, uint32_t mask, volatile uint32_t *addr);

	void operator =(std::nullptr_t);
	void operator =(Poke &&);
	void operator ()();
	operator bool() const;

	void Clear();
	
	~Poke();

	friend void swap(Poke &,Poke &);
	
	void Perform() const;
private:
	void checkBlobMode();

	const void *getValuePtr() const;
	bool ownsBlob() const;

	void PerformBlob() const;
};

void swap(Poke &, Poke &);

class BulkPoke {
	hwPtr addr;

	uint8_t firstScanline;
	uint8_t lastPlusOneScanline;

	uint8_t size;

	union {
		std::unique_ptr<const uint8_t[]> valuePtr8;
		std::unique_ptr<const uint16_t[]> valuePtr16;
		std::unique_ptr<const uint32_t[]> valuePtr32;
	};

public:
	BulkPoke();
	BulkPoke(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint8_t[]> &&values);
	BulkPoke(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint16_t[]> &&values);
	BulkPoke(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint32_t[]> &&values);
	~BulkPoke();

	void Clear();

	void Perform(uint8_t scanline) const;

	BulkPoke(BulkPoke &&);
	BulkPoke &operator=(BulkPoke &&);
};


void swap(BulkPoke &, BulkPoke &);
#endif