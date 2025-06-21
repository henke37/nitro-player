#ifndef POINT_H
#define POINT_H

#include <cstdint>

class Point {
public:
	Point(std::int16_t x, std::int16_t y);
	~Point() = default;
	Point(const Point&) = default;

	Point operator-() const;
	Point operator+(const Point&)const;
	Point operator-(const Point&)const;
	Point operator*(std::int16_t)const;

	Point& operator+=(const Point&);
	Point& operator-=(const Point&);
	Point& operator*=(std::int16_t);

	std::int16_t x, y;
};

Point abs(const Point&);
Point turnCW(const Point&);
Point turnCCW(const Point&);
Point turn180(const Point&);

#endif