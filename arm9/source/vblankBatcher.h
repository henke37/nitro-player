#ifndef VBLANKBATCHER_H
#define VBLANKBATCHER_H

#include "poke.h"

#include <vector>

class VBlankBatcher {
public:
	VBlankBatcher() = default;
	~VBlankBatcher() = default;

	void AddPoke(uint8_t val, volatile uint8_t *addr);
	void AddPoke(uint16_t val, volatile uint16_t *addr);
	void AddPoke(uint32_t val, volatile uint32_t *addr);

	void AddPoke(uint8_t val, volatile uint8_t &ref);
	void AddPoke(uint16_t val, volatile uint16_t &ref);
	void AddPoke(uint32_t val, volatile uint32_t &ref);

	void AddPoke(int8_t val, volatile int8_t *addr);
	void AddPoke(int16_t val, volatile int16_t *addr);
	void AddPoke(int32_t val, volatile int32_t *addr);

	void AddPoke(int8_t val, volatile int8_t &ref);
	void AddPoke(int16_t val, volatile int16_t &ref);
	void AddPoke(int32_t val, volatile int32_t &ref);

	void AddPoke(uint8_t val, uint8_t mask, volatile uint8_t *addr);
	void AddPoke(uint16_t val, uint16_t mask, volatile uint16_t *addr);
	void AddPoke(uint32_t val, uint32_t mask, volatile uint32_t *addr);

	void AddPoke(uint8_t val, uint8_t mask, volatile uint8_t &addr);
	void AddPoke(uint16_t val, uint16_t mask, volatile uint16_t &addr);
	void AddPoke(uint32_t val, uint32_t mask, volatile uint32_t &addr);

	void AddPoke(std::unique_ptr<const uint8_t[]> &&srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	void AddPoke(std::unique_ptr<const uint16_t[]> &&srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	void AddPoke(std::unique_ptr<const uint32_t[]> &&srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	void AddPoke(const uint8_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	void AddPoke(const uint16_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);
	void AddPoke(const uint32_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode);

	void AddPoke(Poke &&p);


	VBlankBatcher(const VBlankBatcher&) = delete;
	void operator=(const VBlankBatcher &) = delete;

	void Clear();
	void Execute() const;

private:
	std::vector<Poke> pokes;
};

#endif