#ifndef FIXEDMATH_H
#define FIXEDMATH_H

#include <cstdint>
#include <cassert>

template <int Base> class FixedPoint;

#define intFromBaseToBase(raw, Base, oldBase) (Base>oldBase) ? (raw >> (Base - oldBase)) : (raw << (oldBase - Base))

template <int Base> class FixedPoint {
public:
	FixedPoint(const int32_t _raw, const int oldBase) noexcept : raw(intFromBaseToBase(_raw, Base, oldBase)) {}
	template <int Base2> FixedPoint(const FixedPoint<Base2> &f2) noexcept : raw(intFromBaseToBase(f2.raw, Base, Base2)) {}
	FixedPoint(const int in) noexcept : raw(in<<Base) {}
	FixedPoint(const double in) noexcept : raw(in * (1<< Base)) {}
	FixedPoint() noexcept {}

	operator int() const noexcept { return raw >> Base; }
	operator uint8_t() const { 
		assert((raw >> Base) < 0x0FF);
		assert((raw >> Base) >= 0);
		return raw >> Base;
	}
	operator int8_t() const {
		assert((raw >> Base) < 0x07F);
		assert((raw >> Base) >= -0x07F);
		return raw >> Base;
	}
	operator uint16_t() const {
		assert((raw >> Base) < 0x0FFFF);
		assert((raw >> Base) >= 0);
		return raw >> Base;
	}
	operator int16_t() const {
		assert((raw >> Base) < 0x07FFF);
		assert((raw >> Base) >= -0x07FFF);
		return raw >> Base;
	}
	operator bool() const noexcept { return (bool)raw; }
	operator float() const noexcept { return ((float)raw) / ((float)(1 << Base)); }
	
	FixedPoint<Base> operator -() const noexcept { return FixedPoint(-raw, Base); }
	FixedPoint<Base> &operator !() const noexcept { return !raw; }
	
	FixedPoint<Base> &operator --() noexcept { raw -= (1 << Base); return *this; }
	FixedPoint<Base> &operator ++() noexcept { raw += (1 << Base); return *this; }

	FixedPoint<Base> &operator =(const FixedPoint<Base> &f2) noexcept { raw = f2.raw; return *this; }
	FixedPoint<Base> &operator +=(const FixedPoint<Base> &f2) noexcept { raw += f2.raw; return *this; }
	FixedPoint<Base> &operator -=(const FixedPoint<Base> &f2) noexcept { raw -= f2.raw; return *this; }
	FixedPoint<Base> &operator *=(const FixedPoint<Base> &f2) noexcept { raw = fixedMul(raw,f2.raw); return *this; }
	FixedPoint<Base> &operator &=(const FixedPoint<Base> &f2) noexcept { raw &= f2.raw; return *this; }
	FixedPoint<Base> &operator |=(const FixedPoint<Base> &f2) noexcept { raw |= f2.raw; return *this; }

	FixedPoint<Base> operator +(const FixedPoint<Base> &f2) const noexcept { return FixedPoint(raw + f2.raw, Base); }
	FixedPoint<Base> operator -(const FixedPoint<Base> &f2) const noexcept { return FixedPoint(raw - f2.raw, Base); }
	FixedPoint<Base> operator *(const FixedPoint<Base> &f2) const noexcept { return FixedPoint(fixedMul(raw,f2.raw), Base); }

	FixedPoint<Base> operator +(const int x) const noexcept { return FixedPoint(raw + (x<<Base), Base); }
	FixedPoint<Base> operator -(const int x) const noexcept { return FixedPoint(raw - (x<<Base), Base); }
	FixedPoint<Base> operator *(const int x) const noexcept { return FixedPoint(fixedMul(raw,x<<Base), Base); }

	FixedPoint<Base> &operator <<=(const unsigned sh) noexcept { raw <<= sh; return *this; }
	FixedPoint<Base> &operator >>=(const unsigned sh) noexcept { raw >>= sh; return *this; }
	FixedPoint<Base> operator <<(const unsigned sh) const noexcept { return FixedPoint(raw << sh, Base); }
	FixedPoint<Base> operator >>(const unsigned sh) const noexcept { return FixedPoint(raw >> sh, Base); }

	bool operator <(const FixedPoint<Base> &f2) const noexcept { return raw < f2.raw; }
	bool operator <=(const FixedPoint<Base> &f2) const noexcept { return raw <= f2.raw; }
	bool operator >(const FixedPoint<Base> &f2) const noexcept { return raw > f2.raw; }
	bool operator >=(const FixedPoint<Base> &f2) const noexcept { return raw >= f2.raw; }
	bool operator !=(const FixedPoint<Base> &f2) const noexcept { return raw != f2.raw; }
	bool operator ==(const FixedPoint<Base> &f2) const noexcept { return raw == f2.raw; }


	bool operator <(const int x) const noexcept { return raw < (x << Base); }
	bool operator <=(const int x) const noexcept { return raw <= (x << Base); }
	bool operator >(const int x) const noexcept { return raw > (x << Base); }
	bool operator >=(const int x) const noexcept { return raw >= (x << Base); }
	bool operator !=(const int x) const noexcept { return raw != (x << Base); }
	bool operator ==(const int x) const noexcept { return raw == (x << Base); }


	FixedPoint<Base> &operator =(const int x) noexcept { raw = (x << Base); return *this; }
	FixedPoint<Base> &operator +=(const int x) noexcept { raw += (x << Base); return *this; }
	FixedPoint<Base> &operator -=(const int x) noexcept { raw -= (x << Base); return *this; }
	FixedPoint<Base> &operator *=(const int x) noexcept { raw *= (x << Base); return *this; }

	int32_t raw;

private:
	int32_t static fixedMul(int32_t a, int32_t b) noexcept {
		return int32_t((int64_t(a) * int64_t(b)) >> Base);
	}

};

template<int Base> bool operator < (const int x, const FixedPoint<Base> &f) noexcept { return (x<<Base) < f.raw; }
template<int Base> bool operator <=(const int x, const FixedPoint<Base> &f) noexcept { return (x<<Base) <= f.raw; }
template<int Base> bool operator > (const int x, const FixedPoint<Base> &f) noexcept { return (x<<Base) > f.raw; }
template<int Base> bool operator >=(const int x, const FixedPoint<Base> &f) noexcept { return (x<<Base) >= f.raw; }
template<int Base> bool operator ==(const int x, const FixedPoint<Base> &f) noexcept { return (x<<Base) == f.raw; }
template<int Base> bool operator !=(const int x, const FixedPoint<Base> &f) noexcept { return (x<<Base) != f.raw; }

template<int Base> FixedPoint<Base> operator +(const int x, const FixedPoint<Base> &f) noexcept { return FixedPoint<Base>((x<<Base) + f.raw, Base); }
template<int Base> FixedPoint<Base> operator -(const int x, const FixedPoint<Base> &f) noexcept { return FixedPoint<Base>((x<<Base) - f.raw, Base); }
template<int Base> FixedPoint<Base> operator *(const int x, const FixedPoint<Base> &f) noexcept { return FixedPoint<Base>(((x<<Base) * f.raw)>>Base, Base); }
template<int Base> FixedPoint<Base> operator &(const int x, const FixedPoint<Base> &f) noexcept { return FixedPoint<Base>((x<<Base) & f.raw, Base); }
template<int Base> FixedPoint<Base> operator |(const int x, const FixedPoint<Base> &f) noexcept { return FixedPoint<Base>((x<<Base) | f.raw, Base); }

typedef FixedPoint<8> fp8;
typedef FixedPoint<12> fp12;


fp8 operator "" _fp8(unsigned long long x) noexcept;
fp12 operator "" _fp12(unsigned long long x) noexcept;
fp12 operator "" _fp12(long double x) noexcept;

fp12 sqrt(const fp12 &x);
fp12 operator / (const fp12 &x, const fp12 &y);
fp12 operator % (const fp12 x, const fp12 y);
fp12 &operator /=(fp12 &x, const fp12 &y);
fp12 &operator %=(fp12 &x, const fp12 &y);


class FixedAngle {
	public:
	FixedAngle();
	FixedAngle(int16_t rawAngle);
	
	operator bool() const noexcept;
	operator float() const noexcept;
	
	FixedAngle &operator =(const FixedAngle &f2) noexcept;
	FixedAngle &operator +=(const FixedAngle &f2) noexcept;
	FixedAngle &operator -=(const FixedAngle &f2) noexcept;
	FixedAngle &operator *=(const FixedAngle &f2) noexcept;
	FixedAngle &operator &=(const FixedAngle &f2) noexcept;
	FixedAngle &operator |=(const FixedAngle &f2) noexcept;

	FixedAngle operator +(const FixedAngle &f2) const noexcept;
	FixedAngle operator -(const FixedAngle &f2) const noexcept;
	FixedAngle operator *(const FixedAngle &f2) const noexcept;
	FixedAngle operator &(const FixedAngle &f2) const noexcept;
	FixedAngle operator |(const FixedAngle &f2) const noexcept;

	FixedAngle &operator <<=(const unsigned sh) noexcept;
	FixedAngle &operator >>=(const unsigned sh) noexcept;
	FixedAngle operator <<(const unsigned sh) const noexcept;
	FixedAngle operator >>(const unsigned sh) const noexcept;

	bool operator <(const FixedAngle &f2) const noexcept;
	bool operator <=(const FixedAngle &f2) const noexcept;
	bool operator >(const FixedAngle &f2) const noexcept;
	bool operator >=(const FixedAngle &f2) const noexcept;
	bool operator !=(const FixedAngle &f2) const noexcept;
	bool operator ==(const FixedAngle &f2) const noexcept;
	
	int16_t raw;
};

FixedAngle operator "" _fixedAngle(unsigned long long x) noexcept;

fp12 sin(const FixedAngle &x);
fp12 cos(const FixedAngle &x);
fp12 tan(const FixedAngle &x);
FixedAngle asin(const fp12 x);
FixedAngle acos(const fp12 x);


#endif