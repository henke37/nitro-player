#ifndef TILEPOS_H
#define TILEPOS_H

constexpr int pixelsPerTilePos = 16;

#include <cstdint>

struct TilePos {
	std::int8_t x,y;
	
	TilePos() = default;
	TilePos(const TilePos &) = default;
	constexpr TilePos(std::int8_t x, std::int8_t y) : x(x), y(y) {}
	
	TilePos operator+(const TilePos &) const noexcept;
	TilePos operator-(const TilePos &) const noexcept;
	
	TilePos operator-() const noexcept;
	TilePos operator*(int8_t) const noexcept;
	TilePos operator/(int8_t) const noexcept;
	
	TilePos &operator=(const TilePos &) = default;
	TilePos &operator+=(const TilePos &) noexcept;
	TilePos &operator-=(const TilePos &) noexcept;
	TilePos &operator*=(std::int8_t) noexcept;
	TilePos &operator/=(std::int8_t) noexcept;
	
	bool operator==(const TilePos &) const noexcept;
	bool operator!=(const TilePos &) const noexcept;
};

#endif