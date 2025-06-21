#include "palMan.h"

#include <stdio.h>

PalMan mainBgPalMan(BG_PALETTE);
PalMan subBgPalMan(BG_PALETTE_SUB);
PalMan mainObjPalMan(SPRITE_PALETTE);
PalMan subObjPalMan(SPRITE_PALETTE_SUB);
ExPalMan mainBgExPalMan(VRAM_E_BG_EXT_PALETTE);

PalMan::PalMan(u16 *palAddr) : palAddr(palAddr), reservations{false}{
}

PalMan::~PalMan() {
}

int PalMan::findEmptySlot() {
	for(int slot=0;slot<16;++slot) {
		if(reservations[slot]) continue;
		
		return slot;
	}
	
	return -1;
}

PalReservation PalMan::reserve() {
	assert(!bigPalMode);

	int slot=findEmptySlot();
	
	assert(slot>=0 && slot<16);
	
	reservations[slot]=true;
	
	return PalReservation(this, slot);
}

FullPalReservation PalMan::reserveFullPal(unsigned int slot) {
	assert(slot == 0);
	assert(!bigPalMode);
	for(int slot = 0; slot < 16; ++slot) {
		assert(!reservations[slot]);
	}
	bigPalMode = true;
	return FullPalReservation(this);
}

u16* PalMan::getDataPtr(unsigned int slot) const {
	assert(slot == 0);
	return palAddr;
}

void PalMan::acquireLock() {}

void PalMan::releaseLock() {}

void PalMan::releaseSlot(unsigned int slot) {
	assert(slot == 0);
	bigPalMode = false;
}

PalReservation::PalReservation() : man(nullptr) {}

PalReservation::PalReservation(PalMan *man, int slot) : man(man), slot(slot) {
	assert(slot>=0 && slot<16);
}

PalReservation::PalReservation(PalReservation &&old) : man(old.man), slot(old.slot) {
	old.man=nullptr;
	old.slot=-1;
}

PalReservation &PalReservation::operator=(PalReservation &&old) {
	if(&old==this) return *this;
	
	assert(man==nullptr);
	
	man=old.man;
	slot=old.slot;
	
	old.man=nullptr;
	old.slot=-1;
	
	return *this;
}

PalReservation::~PalReservation() {
	if(man) {
		man->reservations[slot]=false;
	}
}

void PalReservation::clear() {
	if(man) {
		man->reservations[slot]=false;
	}
	man=nullptr;
	slot=-1;
}

u16* PalReservation::getDataPtr() const {
	assert(man);
	assert(slot>=0 && slot<=15);
	
	return man->palAddr + (slot*16);
}

FullPalReservation::FullPalReservation() : man(nullptr) {}

FullPalReservation::FullPalReservation(PalMan* man) : man(man), slot(0) {
}
FullPalReservation::FullPalReservation(ExPalMan* exMan, unsigned int slot) : man(exMan), slot(slot) {
	assert(slot < exMan->getSlotCount());
}

FullPalReservation::FullPalReservation(FullPalReservation&& old) : man(old.man), slot(old.slot) {
	old.man = nullptr;
}

void FullPalReservation::clear() {
	if(man) {
		man->releaseSlot(slot);
	}
	man = nullptr;
}

FullPalReservation& FullPalReservation::operator=(FullPalReservation&& old) {
	if(&old == this) return *this;

	assert(man == nullptr);

	slot = old.slot;
	man = old.man;

	old.man = nullptr;

	return *this;
}

u16* FullPalReservation::getDataPtr() const {
	assert(man);
	return man->getDataPtr(slot);
}

FullPalReservation::~FullPalReservation() {
	clear();
}

PalMan256::PalMan256() {}

PalMan256::~PalMan256() {}

FullPalReservation ExPalMan::reserveFullPal(unsigned int slot) {
	assert(slot < getSlotCount());
	assert(slot >= skipedSlotCount());
	assert(!reservations[slot]);
	reservations[slot] = true;
	return FullPalReservation(this, slot);
}

LockedFullPalReservation FullPalReservation::acquireLock() {
	assert(man);
	man->acquireLock();
	return LockedFullPalReservation(this);
}

void FullPalReservation::releaseLock() {
	assert(man);
	man->releaseLock();
}

LockedFullPalReservation::LockedFullPalReservation() : reservation(nullptr) {}

LockedFullPalReservation::LockedFullPalReservation(FullPalReservation *reservation) : reservation(reservation) {}

LockedFullPalReservation::~LockedFullPalReservation() {
	if(reservation) {
		release();
	}
}

LockedFullPalReservation::LockedFullPalReservation(LockedFullPalReservation &&old) : reservation(old.reservation) {
	old.reservation = nullptr;
}

LockedFullPalReservation& LockedFullPalReservation::operator=(LockedFullPalReservation &&old) {
	if(&old == this) return *this;

	assert(!this->reservation);

	this->reservation = old.reservation;

	old.reservation = nullptr;

	return *this;
}

void LockedFullPalReservation::release() {
	assert(reservation);
	reservation->releaseLock();
}

u16* LockedFullPalReservation::getDataPtr() const {
	assert(reservation);
	return reservation->getDataPtr();
}

ExPalMan::ExPalMan(VRAM_E_TYPE mappingMode) : bank(VRamBank::E) {
	this->mappingMode.e = mappingMode;
}
ExPalMan::ExPalMan(VRAM_F_TYPE mappingMode) : bank(VRamBank::F) {
	this->mappingMode.f = mappingMode;
}
ExPalMan::ExPalMan(VRAM_G_TYPE mappingMode) : bank(VRamBank::G) {
	this->mappingMode.g = mappingMode;
}
ExPalMan::ExPalMan(VRAM_H_TYPE mappingMode) : bank(VRamBank::H) {
	this->mappingMode.h = mappingMode;
}
ExPalMan::ExPalMan(VRAM_I_TYPE mappingMode) : bank(VRamBank::I) {
	this->mappingMode.i = mappingMode;
}

ExPalMan::~ExPalMan() {}

void ExPalMan::mapLCD() {
	switch(bank) {
	case VRamBank::E:
		vramSetBankE(VRAM_E_LCD);
		break;
	case VRamBank::F:
		vramSetBankF(VRAM_F_LCD);
		break;
	case VRamBank::G:
		vramSetBankG(VRAM_G_LCD);
		break;
	case VRamBank::H:
		vramSetBankH(VRAM_H_LCD);
		break;
	case VRamBank::I:
		vramSetBankI(VRAM_I_LCD);
		break;
	default:
		assert(0);
	}
}

void ExPalMan::mapPal() {
	switch(bank) {
	case VRamBank::E:
		vramSetBankE(mappingMode.e);
		break;
	case VRamBank::F:
		vramSetBankF(mappingMode.f);
		break;
	case VRamBank::G:
		vramSetBankG(mappingMode.g);
		break;
	case VRamBank::H:
		vramSetBankH(mappingMode.h);
		break;
	case VRamBank::I:
		vramSetBankI(mappingMode.i);
		break;
	default:
		assert(0);
	}
}

u16* ExPalMan::getLCDAddr() const {
	switch(bank) {
	case VRamBank::E:
		return VRAM_E;
	case VRamBank::F:
		return VRAM_F;
	case VRamBank::G:
		return VRAM_G;
	case VRamBank::H:
		return VRAM_H;
	case VRamBank::I:
		return VRAM_I;
	default:
		assert(0);
	}
}

unsigned int ExPalMan::skipedSlotCount() const {
	if(bank == VRamBank::F && mappingMode.f == VRAM_F_BG_EXT_PALETTE_SLOT23) {
		return 2;
	} else if(bank == VRamBank::G && mappingMode.g == VRAM_G_BG_EXT_PALETTE_SLOT23) {
		return 2;
	} else {
		return 0;
	}
}

unsigned int ExPalMan::getSlotCount() const {
	switch(bank) {
	case VRamBank::E:
		return 4;
	case VRamBank::F:
		return 2;
	case VRamBank::G:
		return 2;
	case VRamBank::H:
		return 4;
	case VRamBank::I:
		return 1;
	default:
		assert(0);
	}
}

u16* ExPalMan::getDataPtr(unsigned int slot) const {
	size_t bankSize = 256*16;

	u16* dataPtr = getLCDAddr();

	slot -= skipedSlotCount();

	return dataPtr + (bankSize*slot);
}

void ExPalMan::acquireLock() {
	if(lockCount == 0) {
		mapLCD();
	}
	lockCount++;
}
void ExPalMan::releaseLock() {
	lockCount--;
	if(lockCount == 0) {
		mapPal();
	}
}

void ExPalMan::releaseSlot(unsigned int slot) {
	reservations[slot] = false;
}
