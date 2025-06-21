#include "tilePos.h"

TilePos TilePos::operator+(const TilePos &right) const {
	return TilePos(x+right.x, y+right.y);
}

TilePos TilePos::operator-(const TilePos &right) const {
	return TilePos(x-right.x, y-right.y);
}

TilePos TilePos::operator-() const {
	return TilePos(-x, -y);
}

TilePos TilePos::operator*(std::int8_t right) const {
	return TilePos(x*right, y*right);
}

TilePos TilePos::operator/(std::int8_t right) const {
	return TilePos(x/right, y/right);
}

TilePos &TilePos::operator+=(const TilePos &right) {
	x+=right.x;
	y+=right.y;
	
	return *this;
}

TilePos &TilePos::operator-=(const TilePos &right) {
	x-=right.x;
	y-=right.y;
	
	return *this;
}

TilePos &TilePos::operator*=(std::int8_t right) {
	x*=right;
	y*=right;
	
	return *this;
}

TilePos &TilePos::operator/=(std::int8_t right) {
	x/=right;
	y/=right;
	
	return *this;
}

bool TilePos::operator==(const TilePos &right) const {
	return x==right.x && y==right.y;
}

bool TilePos::operator!=(const TilePos &right) const {
	return x!=right.x || y!=right.y;
}