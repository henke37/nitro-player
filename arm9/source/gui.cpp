#include "gui.h"

#include "globals.h"
#include "consts.h"
#include "vblankBatcher.h"

#include <nds.h>

#include <cassert>

GUI gui;

GUI::GUI() {
	tilemap = new TileMapEntry16[32 * 24];
}

GUI::~GUI() {
	delete[] tilemap;
}

void GUI::Load() {  }

void GUI::Unload() {  }

void GUI::addElement(std::unique_ptr<GUIElement>&& element) {
	assert(element);
	if(elements.empty()) {
		vblankBatcher.AddPoke(DISPLAY_BG2_ACTIVE, DISPLAY_BG2_ACTIVE, REG_DISPCNT);
	}
	elements.emplace_back(std::move(element));
	buildTileMap();
}

void GUI::removeElement(GUIElement* element) {
	assert(element);
	for(auto itr = elements.begin(); itr != elements.end(); ++itr) {
		if(itr->get() != element) continue;

		elements.erase(itr);

		if(elements.empty()) {
			vblankBatcher.AddPoke(0, DISPLAY_BG2_ACTIVE, REG_DISPCNT);
		} else {
			buildTileMap();
		}
		return;
	}
	sassert(0,"Unknown gui element %p!", element);
}

void GUI::invalidateTilemap() {
	buildTileMap();
}

void GUI::uploadTilemap() {
	DC_FlushRange(tilemap, 32 * 24 * sizeof(TileMapEntry16));
	dmaCopyHalfWords(3, gui.tilemap, BG_MAP_RAM(bg2MapBase), 32 * 24 * sizeof(TileMapEntry16));
}

void GUI::buildTileMap() {
	clearTilemap();

	for(auto itr = elements.cbegin(); itr != elements.cend(); ++itr) {
		auto &elm = *itr;
		elm->DrawTilemap(tilemap);
	}
	uploadTilemap();
}

GUIElement::~GUIElement() {}


TilePos GUIElement::screenPosToLocalPos(Point pos) const {
	return screenPosToLocalPos(TilePos(pos.x / 8, pos.y / 8));
}

TilePos GUIElement::screenPosToLocalPos(TilePos pos) const {
	return pos - position;
}