#ifndef GLOBALS_H
#define GLOBALS_H

#include <nds/ndstypes.h>

class TileManager;
extern TileManager TileManMainBg0;
extern TileManager TileManMainBg1;
extern TileManager TileManMainBg2;
extern TileManager TileManMainObj;
extern TileManager TileManSubBg0;
extern TileManager TileManSubBg1;
extern TileManager TileManSubObj;

class Point;
extern Point touchPoint;

class GUI;
extern GUI gui;

class ButtonManager;
extern ButtonManager buttonMan;

class VBlankBatcher;
extern VBlankBatcher vblankBatcher;

extern volatile unsigned long int currentFrame;

#endif