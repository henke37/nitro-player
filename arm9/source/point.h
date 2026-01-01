#ifndef POINT_H
#define POINT_H

#include <cstdint>

class Point {
public:
	Point(std::int16_t x, std::int16_t y) noexcept;
	~Point() = default;
	Point(const Point&) = default;

	Point operator-() const noexcept;
	Point operator+(const Point&) const noexcept;
	Point operator-(const Point&) const noexcept;
	Point operator*(std::int16_t) const noexcept;

	Point& operator+=(const Point&) noexcept;
	Point& operator-=(const Point&) noexcept;
	Point& operator*=(std::int16_t) noexcept;

	std::int16_t x, y;
};

Point abs(const Point&) noexcept;
Point turnCW(const Point&) noexcept;
Point turnCCW(const Point&) noexcept;
Point turn180(const Point&) noexcept;

#endif