#include "vblankBatcher.h"

#include <nds/ndstypes.h>

#include "segments.h"

DTCM_BSS VBlankBatcher vblankBatcher;

void VBlankBatcher::AddPoke(uint8_t val, volatile uint8_t *addr) {
	AddPoke(Poke(val, addr));
}
void VBlankBatcher::AddPoke(uint16_t val, volatile uint16_t *addr) {
	AddPoke(Poke(val, addr));
}
void VBlankBatcher::AddPoke(uint32_t val, volatile uint32_t *addr) {
	AddPoke(Poke(val, addr));
}
void VBlankBatcher::AddPoke(uint8_t val, volatile uint8_t &ref) {
	AddPoke(Poke(val, &ref));
}
void VBlankBatcher::AddPoke(uint16_t val, volatile uint16_t &ref) {
	AddPoke(Poke(val, &ref));
}
void VBlankBatcher::AddPoke(uint32_t val, volatile uint32_t &ref) {
	AddPoke(Poke(val, &ref));
}
void VBlankBatcher::AddPoke(std::unique_ptr<const uint8_t[]> &&data, size_t dataSize, hwPtr addr, PokeWriteMode mode) {
	AddPoke(Poke(std::move(data), dataSize, addr, mode));
}
void VBlankBatcher::AddPoke(std::unique_ptr<const uint16_t[]> &&data, size_t dataSize, hwPtr addr, PokeWriteMode mode) {
	AddPoke(Poke(std::move(data), dataSize, addr, mode));
}
void VBlankBatcher::AddPoke(std::unique_ptr<const uint32_t[]> &&data, size_t dataSize, hwPtr addr, PokeWriteMode mode) {
	AddPoke(Poke(std::move(data), dataSize, addr, mode));
}
void VBlankBatcher::AddPoke(const uint8_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode) {
	AddPoke(Poke(srcBuff, dataSize, addr, mode));
}
void VBlankBatcher::AddPoke(const uint16_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode) {
	AddPoke(Poke(srcBuff, dataSize, addr, mode));
}
void VBlankBatcher::AddPoke(const uint32_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode) {
	AddPoke(Poke(srcBuff, dataSize, addr, mode));
}
void VBlankBatcher::AddPoke(uint8_t val, uint8_t mask, volatile uint8_t *addr) {
	AddPoke(Poke(val, mask, addr));
}
void VBlankBatcher::AddPoke(uint16_t val, uint16_t mask, volatile uint16_t *addr) {
	AddPoke(Poke(val, mask, addr));
}
void VBlankBatcher::AddPoke(uint32_t val, uint32_t mask, volatile uint32_t *addr) {
	AddPoke(Poke(val, mask, addr));
}
void VBlankBatcher::AddPoke(uint8_t val, uint8_t mask, volatile uint8_t &addr) {
	AddPoke(Poke(val, mask, &addr));
}
void VBlankBatcher::AddPoke(uint16_t val, uint16_t mask, volatile uint16_t &addr) {
	AddPoke(Poke(val, mask, &addr));
}
void VBlankBatcher::AddPoke(uint32_t val, uint32_t mask, volatile uint32_t &addr) {
	AddPoke(Poke(val, mask, &addr));
}


void VBlankBatcher::AddPoke(Poke &&p) {
	pokes.emplace_back(std::move(p));
}


void VBlankBatcher::Clear() {
	pokes.clear();
}

ITCM_CODE void VBlankBatcher::Execute() const {
	for(auto itr = pokes.cbegin(); itr != pokes.cend(); ++itr) {
		itr->Perform();
	}

}
