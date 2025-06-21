#ifndef GUI_H
#define GUI_H

#include "nclr.h"
#include "ncgr.h"
#include "ncer.h"
#include "nanr.h"
#include "nscr.h"
#include "nftr.h"
#include "animationPlayer.h"

#include "nineslice.h"
#include "tilePos.h"
#include "point.h"

class GUIElement {
public:
	virtual ~GUIElement();
	virtual void DrawTilemap(TileMapEntry16* tilemap) const=0;

	TilePos screenPosToLocalPos(Point pos) const;
	TilePos screenPosToLocalPos(TilePos pos) const;

	TilePos position;
};

class GUI {
public:
	GUI();
	~GUI();

	void Load();
	void Unload();

	void addElement(std::unique_ptr<GUIElement>&& element);
	void removeElement(GUIElement *element);

	void invalidateTilemap();
private:

	std::vector<std::unique_ptr<GUIElement>> elements;

	TileMapEntry16* tilemap;
	void clearTilemap();
	void uploadTilemap();
	void buildTileMap();
};

#endif