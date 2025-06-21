#include "rect.h"

#include <cstdlib>

bool Rect::pointInside(const Point &p) const {
	if(p.x<this->x) return false;
	if(p.x > (signed)(this->x + w)) return false;
	if(p.y < this->y) return false;
	if(p.y > (signed)(this->y + h)) return false;
	return true;
}

bool Rect::overlaps(const Rect& other) const {
	if(getRight() <= other.x) return false;
	if(other.getRight() <= x) return false;
	if(getBottom() <= other.y) return false;
	if(other.getBottom() <= y) return false;
	return true;
}

bool Rect::hasArea() const {
	return w>0 && h>0;
}

unsigned int Rect::getArea() const {
	return w*h;
}

Point Rect::distanceInside(const Point &p1) {
	return Point(p1.x-x,p1.y-y);
}

Point Rect::getRandomPoint() const {
	return Point(x+rand()%w, y+rand()%h);
}
