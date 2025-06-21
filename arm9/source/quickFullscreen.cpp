#include "quickFullscreen.h"

void quickFullscreen(TileMapEntry16 *map) {
	quickFullscreen(map, 0, 0, 32, 24, 0);
}
void quickFullscreen(TileMapEntry16 *map, int startTileIndex) {
	quickFullscreen(map, 0, 0, 32, 24, startTileIndex, 0);
}
void quickFullscreen(TileMapEntry16 *map, int startTileIndex, uint8_t palette) {
	quickFullscreen(map, 0, 0, 32, 24, startTileIndex, palette);
}
void quickFullscreen(TileMapEntry16 *map, uint8_t startX, uint8_t startY, uint8_t width, uint8_t height) {
	quickFullscreen(map, startX, startY, width, height, 0, 0);
}
void quickFullscreen(TileMapEntry16 *map, uint8_t startX, uint8_t startY, uint8_t width, uint8_t height, int startTileIndex) {
	quickFullscreen(map, startX, startY, width, height, startTileIndex, 0);
}

void quickFullscreen(TileMapEntry16 *map, uint8_t startX, uint8_t startY, uint8_t width, uint8_t height, int startTileIndex, uint8_t palette) {
	assert(startX + width <= 32);
	assert(startY + height <= 24);
	assert(palette < 16);
	assert(startTileIndex >= 0);
	assert(startTileIndex + width * height <= 0x00FFFF);

	for(int y=0;y<height;++y) {
		for(int x=0;x<width;++x) {
			TileMapEntry16 tile;
			tile.index=x+y*width+startTileIndex;
			tile.hflip=0;
			tile.vflip=0;
			tile.palette=palette;
			map[x+startX+(y+startY)*32]=tile;
		}
	}
}