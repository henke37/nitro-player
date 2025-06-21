#ifndef TILEMAPMIRROR_H
#define TILEMAPMIRROR_H

#include "poke.h"
#include "tilePos.h"

#include <nds.h>

#include <memory>

class TileMapMirror {
public:
	TileMapMirror(std::uint8_t width, std::uint8_t height, u16 *baseAddr);
	~TileMapMirror();

	void clear(TileMapEntry16 tile);
	void setTile(TilePos pos, TileMapEntry16 tile);

	TileMapEntry16 *get() { return mirror.get(); }

	TileMapMirror(const TileMapMirror &) = delete;
	void operator=(const TileMapMirror &) = delete;
	
	Poke prepareUpload(PokeWriteMode mode) const;
	void queueUpload(PokeWriteMode mode = PokeWriteMode::DMA_16) const;
	void uploadNow(PokeWriteMode mode = PokeWriteMode::DMA_16) const;
private:
	unsigned int width, height;

	size_t getTileCount() const { return width * height; }

	size_t getBuffSize() const { return getTileCount() * sizeof(TileMapEntry16); }

	unsigned int posToTileIndex(TilePos pos) const;

	std::unique_ptr<TileMapEntry16[]> mirror;

	u16 *baseAddr;
};

#endif