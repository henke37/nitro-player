#include "point.h"

Point::Point(std::int16_t x, std::int16_t y) : x(x), y(y){}

Point Point::operator-() const {
    return Point(-x,-y);
}
Point Point::operator+(const Point &p2) const {
    return Point(x+p2.x,y+p2.y);
}
Point Point::operator-(const Point& p2) const {
    return Point(x - p2.x, y - p2.y);
}
Point Point::operator*(std::int16_t v) const {
    return Point(x*v,y*v);
}
Point& Point::operator+=(const Point &p2) {
    x += p2.x;
    y += p2.y;
    return *this;
}
Point& Point::operator-=(const Point& p2) {
    x -= p2.x;
    y -= p2.y;
    return *this;
}
Point turnCW(const Point &p) {
    return Point(-p.y, p.x);
}
Point turnCCW(const Point& p) {
    return Point(p.y, -p.x);
}
Point turn180(const Point& p) {
    return Point(-p.x, -p.y);
}