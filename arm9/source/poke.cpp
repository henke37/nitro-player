#include "poke.h"
#include <nds/arm9/sassert.h>
#include <cassert>

Poke::Poke() : size(0), type(PokeType::NOOP) {}
Poke::Poke(std::uint8_t val, volatile std::uint8_t *addr_) : size(sizeof(std::uint8_t)), type(PokeType::INT), addr(addr_), value8(val) {
}
Poke::Poke(std::uint16_t val, volatile std::uint16_t *addr_) : size(sizeof(std::uint16_t)), type(PokeType::INT), addr(addr_), value16(val) {
}
Poke::Poke(std::uint32_t val, volatile std::uint32_t *addr_) : size(sizeof(std::uint32_t)), type(PokeType::INT), addr(addr_), value32(val) {
}
Poke::Poke(std::int8_t val, volatile std::int8_t *addr_) : size(sizeof(uint8_t)), type(PokeType::INT), addr(addr_), value8(val) {}
Poke::Poke(std::int16_t val, volatile std::int16_t *addr_) : size(sizeof(uint16_t)), type(PokeType::INT), addr(addr_), value16(val) {}
Poke::Poke(std::int32_t val, volatile std::int32_t *addr_) : size(sizeof(uint32_t)), type(PokeType::INT), addr(addr_), value32(val) {}

Poke::Poke(std::unique_ptr<const std::uint8_t[]> &&dataPtr, size_t dataSize, hwPtr addr_, PokeWriteMode mode) : size(dataSize), writeMode(mode), type(PokeType::OWNBLOB_8), addr(addr_), valuePtr8(std::move(dataPtr)) {
	checkBlobMode();
}
Poke::Poke(std::unique_ptr<const std::uint16_t[]> &&dataPtr, size_t dataSize, hwPtr addr_, PokeWriteMode mode) : size(dataSize), writeMode(mode), type(PokeType::OWNBLOB_16), addr(addr_), valuePtr16(std::move(dataPtr)) {
	checkBlobMode();
}
Poke::Poke(std::unique_ptr<const std::uint32_t[]> &&dataPtr, size_t dataSize, hwPtr addr_, PokeWriteMode mode) : size(dataSize), writeMode(mode), type(PokeType::OWNBLOB_32), addr(addr_), valuePtr32(std::move(dataPtr)) {
	checkBlobMode();
}

Poke::Poke(const std::uint8_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode) : size(dataSize), writeMode(mode), type(PokeType::RAWBLOB_8), addr(addr), rawPtr8(srcBuff) {
	checkBlobMode();
}

Poke::Poke(const std::uint16_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode) : size(dataSize), writeMode(mode), type(PokeType::RAWBLOB_16), addr(addr), rawPtr16(srcBuff) {
	checkBlobMode();
}

Poke::Poke(const std::uint32_t *srcBuff, size_t dataSize, hwPtr addr, PokeWriteMode mode) : size(dataSize), writeMode(mode), type(PokeType::RAWBLOB_32), addr(addr), rawPtr32(srcBuff) {
	checkBlobMode();
}

Poke::Poke(std::uint8_t val, std::uint8_t mask, volatile std::uint8_t *addr_) : size(sizeof(std::uint8_t)), type(PokeType::BITFIELD), addr(addr_), bitField8(val, mask) {
}
Poke::Poke(std::uint16_t val, std::uint16_t mask, volatile std::uint16_t *addr_) : size(sizeof(std::uint16_t)), type(PokeType::BITFIELD), addr(addr_), bitField16(val, mask) {
}
Poke::Poke(std::uint32_t val, std::uint32_t mask, volatile std::uint32_t *addr_) : size(sizeof(std::uint32_t)), type(PokeType::BITFIELD), addr(addr_), bitField32(val,mask) {
}

Poke::Poke(Poke &&p2) : size(p2.size), writeMode(p2.writeMode), type(p2.type), addr(p2.addr) {
	assert(&p2 != this);

	switch(p2.type) {
		case PokeType::NOOP:
		break;
		case PokeType::INT:
			switch(p2.size) {
				case sizeof(uint8_t):
					value8=p2.value8;
				break;
				case sizeof(uint16_t):
					value16=p2.value16;
				break;
				case sizeof(uint32_t):
					value32=p2.value32;
				break;
				default:
					sassert(0,"Invalid size %i for plain poke found", p2.size);
			}
		break;
		
		case PokeType::BITFIELD:
			switch (p2.size) {
				case sizeof(uint8_t) :
					std::construct_at(&bitField8, std::move(p2.bitField8));
					break;
				case sizeof(uint16_t) :
					std::construct_at(&bitField16, std::move(p2.bitField16));
					break;
				case sizeof(uint32_t) :
					std::construct_at(&bitField32, std::move(p2.bitField32));
					break;
				default:
					sassert(0,"Invalid size %i found for bitfield poke", p2.size);
			}
		break;
		
		case PokeType::OWNBLOB_8:
				std::construct_at(&valuePtr8, std::move(p2.valuePtr8));
			break;
		case PokeType::RAWBLOB_8:
				std::construct_at(&rawPtr8, std::move(p2.rawPtr8));
			break;
		case PokeType::OWNBLOB_16:
			std::construct_at(&valuePtr16, std::move(p2.valuePtr16));
			break;
		case PokeType::RAWBLOB_16:
			std::construct_at(&rawPtr16, std::move(p2.rawPtr16));
			break;
		case PokeType::OWNBLOB_32:
			std::construct_at(&valuePtr32, std::move(p2.valuePtr32));
			break;
		case PokeType::RAWBLOB_32:
			std::construct_at(&rawPtr32, std::move(p2.rawPtr32));
			break;
	}
	p2.Clear();
}

void Poke::operator=(std::nullptr_t) {
	Clear();
}

Poke::operator bool() const {
	return type != PokeType::NOOP;
}

void Poke::operator=(Poke &&p2) {
	assert(this != &p2);
	Clear();

	size = p2.size;
	type = p2.type;
	writeMode = p2.writeMode;
	addr = p2.addr;

	switch (p2.type) {
		case PokeType::NOOP:
			break;
		case PokeType::INT:
			switch (p2.size) {
				case sizeof(std::uint8_t) :
					value8 = p2.value8;
					break;
				case sizeof(std::uint16_t) :
					value16 = p2.value16;
					break;
				case sizeof(std::uint32_t) :
					value32 = p2.value32;
					break;
				default:
					sassert(0, "Invalid size %i for plain poke found", p2.size);
			}
			break;

		case PokeType::BITFIELD:
			switch (p2.size) {
				case sizeof(std::uint8_t) :
					std::construct_at(&bitField8, std::move(p2.bitField8));
					break;
				case sizeof(std::uint16_t) :
					std::construct_at(&bitField16, std::move(p2.bitField16));
					break;
				case sizeof(std::uint32_t) :
					std::construct_at(&bitField32, std::move(p2.bitField32));
					break;
				default:
					sassert(0, "Invalid size %i found for bitfield poke", p2.size);
			}
			break;

		case PokeType::OWNBLOB_8:
			std::construct_at(&valuePtr8, std::move(p2.valuePtr8));
			break;
		case PokeType::RAWBLOB_8:
			std::construct_at(&rawPtr8, std::move(p2.rawPtr8));
			break;
		case PokeType::OWNBLOB_16:
			std::construct_at(&valuePtr16, std::move(p2.valuePtr16));
			break;
		case PokeType::RAWBLOB_16:
			std::construct_at(&rawPtr16, std::move(p2.rawPtr16));
			break;
		case PokeType::OWNBLOB_32:
			std::construct_at(&valuePtr32, std::move(p2.valuePtr32));
			break;
		case PokeType::RAWBLOB_32:
			std::construct_at(&rawPtr32, std::move(p2.rawPtr32));
			break;
	}
	p2.Clear();
}

void Poke::Clear() {
	switch (type) {
		case PokeType::OWNBLOB_8:
			valuePtr8.~unique_ptr();
		break;
		case PokeType::OWNBLOB_16:
			valuePtr16.~unique_ptr();
		break;
		case PokeType::OWNBLOB_32:
			valuePtr32.~unique_ptr();
		break;
		case PokeType::BITFIELD://no destructor to call
		default:
			break;
	}
	type = PokeType::NOOP;
}


Poke::~Poke() {
	Clear();
}

void swap(Poke &p1, Poke &p2) {
	Poke swapTemp = std::move(p1);
	p1 = std::move(p2);
	p2 = std::move(swapTemp);	
}

void Poke::checkBlobMode() {
	switch(type) {
	case PokeType::RAWBLOB_8:
	case PokeType::RAWBLOB_16:
	case PokeType::RAWBLOB_32:
	case PokeType::OWNBLOB_8:
	case PokeType::OWNBLOB_16:
	case PokeType::OWNBLOB_32:
		switch(writeMode) {
		case PokeWriteMode::MEMCPY_8:
		case PokeWriteMode::MEMCPY_16:
		case PokeWriteMode::MEMCPY_32:
		case PokeWriteMode::DMA_16:
		case PokeWriteMode::DMA_32:
			break;
		case PokeWriteMode::INVALID:
		default:
			sassert(0, "Invalid write mode %i selected.", (int)writeMode);
		}
		break;
	default:
		sassert(0, "Invalid poketype %i selected.", (int)type);
	}
}

BulkPoke::BulkPoke() : addr(nullptr), size(0) {}

BulkPoke::BulkPoke(hwPtr addr, std::uint8_t firstScanline, std::uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint8_t[]> &&values) :
	addr(addr), firstScanline(firstScanline), lastPlusOneScanline(lastPlusOneScanline), size(8), valuePtr8(std::move(values)) {}
BulkPoke::BulkPoke(hwPtr addr, std::uint8_t firstScanline, std::uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint16_t[]> &&values) :
	addr(addr), firstScanline(firstScanline), lastPlusOneScanline(lastPlusOneScanline), size(16), valuePtr16(std::move(values)) {}
BulkPoke::BulkPoke(hwPtr addr, std::uint8_t firstScanline, std::uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint32_t[]> &&values) :
	addr(addr), firstScanline(firstScanline), lastPlusOneScanline(lastPlusOneScanline), size(32), valuePtr32(std::move(values)) {}

BulkPoke::~BulkPoke() {
	Clear();
}

void BulkPoke::Clear() {
	switch(size) {
	case 0: break;
	case 8:
		valuePtr8.~unique_ptr();
		break;
	case 16:
		valuePtr16.~unique_ptr();
		break;
	case 32:
		valuePtr32.~unique_ptr();
		break;
	default:
		sassert(0,"Invalid bulk poke size %i", size);
	}

	size = 0;
	addr = nullptr;
}

BulkPoke::BulkPoke(BulkPoke &&old) : 
	addr(old.addr),
	firstScanline(old.firstScanline),
	lastPlusOneScanline(old.lastPlusOneScanline),
	size(old.size) {

	switch(size) {
	case 0: break;
	case 8:
		std::construct_at(&valuePtr8, std::move(old.valuePtr8));
		break;
	case 16:
		std::construct_at(&valuePtr16, std::move(old.valuePtr16));
		break;
	case 32:
		std::construct_at(&valuePtr32, std::move(old.valuePtr32));
		break;
	default:
		sassert(0, "Invalid bulk poke size %i", size);
	}

	old.Clear();
}

BulkPoke &BulkPoke::operator=(BulkPoke &&old) {
	Clear();

	addr = old.addr;
	firstScanline = old.firstScanline;
	lastPlusOneScanline = old.lastPlusOneScanline;
	size = old.size;

	switch(size) {
	case 0: break;
	case 8:
		std::construct_at(&valuePtr8, std::move(old.valuePtr8));
		break;
	case 16:
		std::construct_at(&valuePtr16, std::move(old.valuePtr16));
		break;
	case 32:
		std::construct_at(&valuePtr32, std::move(old.valuePtr32));
		break;
	default:
		sassert(0, "Invalid bulk poke size %i", size);
	}

	old.Clear();

	return *this;
}

