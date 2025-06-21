#include "tileMan.h"

#include <algorithm>
#include <cassert>

#include "globals.h"
#include "consts.h"

TileManager TileManMainBg0(bg0TileBase, tileBaseMainBig, 1024, 4);
TileManager TileManMainBg1(bg1TileBase, tileBaseMainBig, 1024, 4);
TileManager TileManMainBg2(bg2TileBase, tileBaseMainBig, 1024, 4);
TileManager TileManMainBg3(bg3TileBase, tileBaseMainBig, 1024, 4);
TileManager TileManMainObj(SPRITE_GFX,4096,4);
TileManager TileManSubBg0(BG_TILE_RAM_SUB(subBg0TileBase),1024,4);
TileManager TileManSubBg1(BG_TILE_RAM_SUB(subBg1TileBase),1024,4);
TileManager TileManSubObj(SPRITE_GFX_SUB,4096,4);

TileManager::TileManager(void *addr, int maxTiles, int bpp) : addr(addr), bpp(bpp), maxTiles(maxTiles), slots() {
	assert(bpp==4 || bpp==8);
	assert(maxTiles > 0);
}

TileManager::TileManager(unsigned int tileBase, unsigned int bigBase, int maxTiles, int bpp) : bpp(bpp), maxTiles(maxTiles), slots() {
	assert(bpp == 4 || bpp == 8);
	assert(maxTiles > 0);
	uintptr_t tileAddr = 0x06000000 + tileBase * 0x4000 + bigBase * 0x10000;
	addr = (u16*)(tileAddr);
}

TileManager::~TileManager() {}

std::vector<TileReservationSlot>::iterator TileManager::findFirstGap(int tileCount) {
	assert(tileCount <= maxTiles);
	for(auto itr = slots.begin(); itr!= slots.end()-1; ++itr) {
		auto nextItr = itr+1;
		
		int spareRoom = nextItr->startTileIndex - itr->endTileIndex();
		
		if(spareRoom < tileCount) continue;
		
		return itr;
	}
	
	sassert(slots.back().endTileIndex() + tileCount <= maxTiles, "No gap of at least %i found!",tileCount);
	
	return slots.end()-1;
}

TileReservationToken TileManager::reserve(int tileCount) {
	assert(tileCount > 0);
	
	int startIndex;
	
	if(slots.empty()) {
		sassert(tileCount<=maxTiles, "Can't fit %i! Max %i", tileCount, maxTiles);
		startIndex=0;
		slots.emplace_back(TileReservationSlot(startIndex,tileCount));
	} else {
		auto startItr = findFirstGap(tileCount);
		startIndex = startItr->endTileIndex();
		slots.emplace(startItr+1, TileReservationSlot(startIndex,tileCount));
	}
	
	return TileReservationToken(this, startIndex);
}

void TileManager::setBpp(int bpp) {
	assert(slots.empty());
	this->bpp = bpp;
}

void TileManager::dump() const {
	printf("Tileman %p", addr);
}

void TileManager::unreserve(int startTileIndex) {
	for(auto itr = slots.begin(); itr!= slots.end(); ++itr) {
		if(itr->startTileIndex!=startTileIndex) continue;
		itr=slots.erase(itr);
		return;
	}
	sassert(0, "No slot with start %i",startTileIndex);
}

TileReservationToken::TileReservationToken(TileManager *man, int startTileIndex) : man(man), startTileIndex(startTileIndex) {}

TileReservationToken::TileReservationToken() : man(nullptr), startTileIndex(-1) {}
TileReservationToken::TileReservationToken(TileReservationToken &&old) : man(old.man), startTileIndex(old.startTileIndex) {
	old.man=nullptr;
	old.startTileIndex=-1;
}

TileReservationToken::~TileReservationToken() {
	if(man) {
		man->unreserve(startTileIndex);
	}
}

TileReservationToken &TileReservationToken::operator=(TileReservationToken &&old) {
	if(&old == this) return *this;

	assert(!man);
	
	man=old.man;
	startTileIndex=old.startTileIndex;
	
	old.man=nullptr;
	old.startTileIndex=-1;
	
	return *this;
}

int TileReservationToken::getStartTileIndex() const {
	assert(man);
	assert(man->bpp==4 || man->bpp==8);
	return startTileIndex;
}

u8 *TileReservationToken::getDataPtr() const {
	assert(man);
	assert(man->bpp==4 || man->bpp==8);
	
	size_t tileSize = (man->bpp==8?8:4) * 8;
	
	return ((u8*)man->addr) + (tileSize * startTileIndex);
}