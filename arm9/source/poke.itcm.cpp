#include "poke.h"
#include "registerOverride.h"
#include <nds/dma.h>
#include <nds/arm9/video.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/cache.h>

#define VRAM_E_SIZE (64*1024)
#define VRAM_F_SIZE (16*1024)
#define VRAM_G_SIZE (16*1024)
#define VRAM_H_SIZE (32*1024)
#define VRAM_I_SIZE (16*1024)

#include "tcm.h"

ITCM_CODE static constexpr bool pointerInRange(volatile void *needle, volatile void *base, size_t size);

ITCM_CODE void Poke::operator()() {
	Perform();
}

ITCM_CODE void Poke::Perform() const {
	if (type == PokeType::NOOP) return;
	registerOverride<uint8_t> oldVRamMode;

	if(pointerInRange(addr, VRAM_E, VRAM_E_SIZE)) {
		oldVRamMode.set(&VRAM_E_CR, VRAM_ENABLE | VRAM_E_LCD);
	} else if(pointerInRange(addr, VRAM_F, VRAM_F_SIZE)) {
		oldVRamMode.set(&VRAM_F_CR, VRAM_ENABLE | VRAM_F_LCD);
	} else if(pointerInRange(addr, VRAM_G, VRAM_G_SIZE)) {
		oldVRamMode.set(&VRAM_G_CR, VRAM_ENABLE | VRAM_G_LCD);
	} else if(pointerInRange(addr, VRAM_H, VRAM_H_SIZE)) {
		oldVRamMode.set(&VRAM_H_CR, VRAM_ENABLE | VRAM_H_LCD);
	} else if(pointerInRange(addr, VRAM_I, VRAM_I_SIZE)) {
		oldVRamMode.set(&VRAM_I_CR, VRAM_ENABLE | VRAM_I_LCD);
	}

	switch(type) {
	case PokeType::NOOP:
		break;
	case PokeType::INT:
		switch(size) {
		case sizeof(uint8_t) :
			*((volatile uint8_t*)addr) = value8;
			break;
		case sizeof(uint16_t) :
			*((volatile uint16_t*)addr) = value16;
			break;
		case sizeof(uint32_t) :
			*((volatile uint32_t*)addr) = value32;
			break;
		default:
			sassert(0,"Invalid size %i found for plain poke", size);
		}
		break;
	case PokeType::BITFIELD:
		switch(size) {
		case sizeof(uint8_t) :
			bitField8.Poke((volatile uint8_t*)addr);
			break;
		case sizeof(uint16_t) :
			bitField16.Poke((volatile uint16_t*)addr);
			break;
		case sizeof(uint32_t) :
			bitField32.Poke((volatile uint32_t*)addr);
			break;
		default:
			sassert(0,"Invalid size %i found for bitfield poke", size);
		}
		break;

	case PokeType::RAWBLOB_8:
	case PokeType::RAWBLOB_16:
	case PokeType::RAWBLOB_32:
	case PokeType::OWNBLOB_8:
	case PokeType::OWNBLOB_16:
	case PokeType::OWNBLOB_32:
		PerformBlob();
	break;

	default:
		sassert(0, "Invalid poke type %i", (int)type);
	}
}

static constexpr bool pointerInRange(uintptr_t needle, uintptr_t base, size_t size) {
	return needle >= base && needle < (base + size);
}
static constexpr bool pointerInRange(volatile void *needle, volatile void *base, size_t size) {
	return pointerInRange((uintptr_t)needle, (uintptr_t)base, size);
}

ITCM_CODE const void *Poke::getValuePtr() const {
	switch(type) {
	case PokeType::RAWBLOB_8:
		return rawPtr8;
	case PokeType::RAWBLOB_16:
		return rawPtr16;
	case PokeType::RAWBLOB_32:
		return rawPtr32;
	case PokeType::OWNBLOB_8:
		return valuePtr8.get();
	case PokeType::OWNBLOB_16:
		return valuePtr16.get();
	case PokeType::OWNBLOB_32:
		return valuePtr32.get();
	default:
		return nullptr;
	}
}

ITCM_CODE void Poke::PerformBlob() const {
	const void *valuePtr = getValuePtr();
	switch(this->writeMode) {
		case PokeWriteMode::DMA_16:
			DC_FlushRange(valuePtr, size);
			dmaCopyHalfWords(0, valuePtr, (void *)addr, size);
			break;
		case PokeWriteMode::DMA_32:
			DC_FlushRange(valuePtr, size);
			dmaCopyHalfWords(0, valuePtr, (void *)addr, size);
		case PokeWriteMode::MEMCPY_8:
		{
			volatile const uint8_t *src = (volatile const uint8_t *)valuePtr;
			std::copy(src, src + size, (uint8_t *)addr);
		} break;
		case PokeWriteMode::MEMCPY_16:
		{
			volatile const uint16_t *src = (volatile const uint16_t *)valuePtr;
			std::copy(src, src + size / 2, (uint16_t *)addr);
		} break;
		case PokeWriteMode::MEMCPY_32:
		{
			volatile const uint32_t *src = (volatile const uint32_t *)valuePtr;
			std::copy(src, src + size / 4, (uint32_t *)addr);
		} break;
		case PokeWriteMode::INVALID:
		default:
			sassert(0, "Invalid writemode %i", (int)writeMode);
	}
}

ITCM_CODE void BulkPoke::Perform(uint8_t scanline) const {
	if(scanline < firstScanline) return;
	if(scanline >= lastPlusOneScanline) return;

	uint8_t entryIndex = scanline - firstScanline;
	switch(size) {
	case 0: break;
	case 8:
		*static_cast<volatile uint8_t *>(addr) = valuePtr8[entryIndex];
		break;
	case 16:
		*static_cast<volatile uint16_t *>(addr) = valuePtr16[entryIndex];
		break;
	case 32:
		*static_cast<volatile uint32_t *>(addr) = valuePtr32[entryIndex];
		break;
	default:
		sassert(0, "Invalid bulk poke size %i", size);
	}
}