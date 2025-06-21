#ifndef RECT_H
#define RECT_H

#include "point.h"

class Rect {
public:
	constexpr Rect(std::int16_t x, std::int16_t y, std::uint16_t w, std::uint16_t h) : x(x), y(y), w(w), h(h) {}
	~Rect() = default;
	Rect(const Rect&) = default;

	bool pointInside(const Point&) const;
	bool overlaps(const Rect& other) const;
	bool fullyContains(const Rect& other) const;
	bool hasArea() const;
	unsigned int getArea() const;
	void expandToContain(const Rect&);
	void expandToContain(const Point&);
	Point distanceInside(const Point&);

	Point getRandomPoint() const;

	constexpr std::int16_t getRight() const { return x + w; }
	constexpr std::int16_t getBottom() const { return y + h; }
	Point getCenter() const { return Point(x + w / 2, y + h / 2); }

	std::int16_t x, y;
	std::uint16_t w, h;
};

#endif