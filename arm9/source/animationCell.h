#ifndef ANIMATION_CELL_H
#define ANIMATION_CELL_H

#include <nds.h>
#include <string>
#include <vector>

enum class MappingMode {
	CharMapping_1D_32,
	CharMapping_1D_64,
	CharMapping_1D_128,
	CharMapping_1D_256,
	CharMapping_2D
};

class AnimationCell {
public:
	std::vector<SpriteEntry> objectDefs;
	
	uint16_t atts;
	
	uint16_t minX;
	uint16_t minY;
	uint16_t maxX;
	uint16_t maxY;
};

class CellBank {
public:
	const AnimationCell &getCell(int index) const;
	AnimationCell &getCell(int index);
	
	const AnimationCell &getCell(const std::string &name) const;
	AnimationCell &getCell(const std::string &name);

	MappingMode getMappingMode() const { return mappingMode; }
	
protected:
	std::vector<AnimationCell> cells;

	MappingMode mappingMode;
};

#endif