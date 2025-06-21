#ifndef TILEPOS_H
#define TILEPOS_H

constexpr int pixelsPerTilePos = 16;

#include <cstdint>

struct TilePos {
	std::int8_t x,y;
	
	TilePos() = default;
	TilePos(const TilePos &) = default;
	constexpr TilePos(std::int8_t x, std::int8_t y) : x(x), y(y) {}
	
	TilePos operator+(const TilePos &) const;
	TilePos operator-(const TilePos &) const;
	
	TilePos operator-() const;
	TilePos operator*(int8_t) const;
	TilePos operator/(int8_t) const;
	
	TilePos &operator=(const TilePos &) = default;
	TilePos &operator+=(const TilePos &);
	TilePos &operator-=(const TilePos &);
	TilePos &operator*=(std::int8_t);
	TilePos &operator/=(std::int8_t);
	
	bool operator==(const TilePos &) const;
	bool operator!=(const TilePos &) const;
};

#endif