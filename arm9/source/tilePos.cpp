#include "tilePos.h"

TilePos TilePos::operator+(const TilePos &right) const noexcept {
	return TilePos(x+right.x, y+right.y);
}

TilePos TilePos::operator-(const TilePos &right) const noexcept {
	return TilePos(x-right.x, y-right.y);
}

TilePos TilePos::operator-() const noexcept {
	return TilePos(-x, -y);
}

TilePos TilePos::operator*(std::int8_t right) const noexcept {
	return TilePos(x*right, y*right);
}

TilePos TilePos::operator/(std::int8_t right) const noexcept {
	return TilePos(x/right, y/right);
}

TilePos &TilePos::operator+=(const TilePos &right) noexcept {
	x+=right.x;
	y+=right.y;
	
	return *this;
}

TilePos &TilePos::operator-=(const TilePos &right) noexcept {
	x-=right.x;
	y-=right.y;
	
	return *this;
}

TilePos &TilePos::operator*=(std::int8_t right) noexcept {
	x*=right;
	y*=right;
	
	return *this;
}

TilePos &TilePos::operator/=(std::int8_t right) noexcept {
	x/=right;
	y/=right;
	
	return *this;
}

bool TilePos::operator==(const TilePos &right) const noexcept {
	return x==right.x && y==right.y;
}

bool TilePos::operator!=(const TilePos &right) const noexcept {
	return x!=right.x || y!=right.y;
}