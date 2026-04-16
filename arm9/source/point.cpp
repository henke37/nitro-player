#include "point.h"

Point::Point(std::int16_t x, std::int16_t y) noexcept : x(x), y(y){}

Point Point::operator-() const noexcept {
    return Point(-x,-y);
}
Point Point::operator+(const Point &p2) const noexcept {
    return Point(x+p2.x,y+p2.y);
}
Point Point::operator-(const Point& p2) const noexcept {
    return Point(x - p2.x, y - p2.y);
}
Point Point::operator*(std::int16_t v) const noexcept {
    return Point(x*v,y*v);
}
Point& Point::operator+=(const Point &p2) noexcept {
    x += p2.x;
    y += p2.y;
    return *this;
}
Point& Point::operator-=(const Point& p2) noexcept {
    x -= p2.x;
    y -= p2.y;
    return *this;
}
Point turnCW(const Point &p) noexcept {
    return Point(-p.y, p.x);
}
Point turnCCW(const Point& p) noexcept {
    return Point(p.y, -p.x);
}
Point turn180(const Point& p) noexcept {
    return Point(-p.x, -p.y);
}