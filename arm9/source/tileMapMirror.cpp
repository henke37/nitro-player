#include "tileMapMirror.h"

#include "globals.h"
#include "vblankBatcher.h"

#include <cassert>

TileMapMirror::TileMapMirror(std::uint8_t width, std::uint8_t height, u16 *baseAddr) : 
	width(width), 
	height(height),
	baseAddr(baseAddr)
{
	assert(width > 0 && width <= 64);
	assert(height > 0 && height <= 64);

	mirror = std::make_unique<TileMapEntry16[]>(getTileCount());
}

TileMapMirror::~TileMapMirror() {}

void TileMapMirror::clear(TileMapEntry16 tile) {
	unsigned int tileCount = width * height;
	for(unsigned int index = 0; index < tileCount; ++index) {
		mirror[index] = tile;
	}
}

void TileMapMirror::setTile(TilePos pos, TileMapEntry16 tile) {
	auto index = posToTileIndex(pos);
	mirror[index] = tile;
}

Poke TileMapMirror::prepareUpload(PokeWriteMode mode) const {
	return Poke((const uint16_t *)mirror.get(), getBuffSize(), baseAddr, mode);
}

void TileMapMirror::queueUpload(PokeWriteMode mode) const {
	vblankBatcher.AddPoke(std::move(prepareUpload(mode)));
}

void TileMapMirror::uploadNow(PokeWriteMode mode) const {
	prepareUpload(mode).Perform();
}
