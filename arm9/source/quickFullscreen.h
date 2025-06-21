#ifndef QUICKFULLSCREEN_H
#define QUICKFULLSCREEN_H

#include <nds.h>

void quickFullscreen(TileMapEntry16 *map);
void quickFullscreen(TileMapEntry16 *map, int startTileIndex);
void quickFullscreen(TileMapEntry16 *map, int startTileIndex, uint8_t palette);
void quickFullscreen(TileMapEntry16 *map, uint8_t startX, uint8_t startY, uint8_t width, uint8_t height);
void quickFullscreen(TileMapEntry16 *map, uint8_t startX, uint8_t startY, uint8_t width, uint8_t height, int startTileIndex);
void quickFullscreen(TileMapEntry16 *map, uint8_t startX, uint8_t startY, uint8_t width, uint8_t height, int startTileIndex, uint8_t palette);

#endif