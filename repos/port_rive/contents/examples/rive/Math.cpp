
#include "RMath.h"

using namespace ld;

// Constants
const Vec2 Vec2::zero(0.f, 0.f);
const Vec2 Vec2::one(1.f, 1.f);
const Rect Rect::zero(0.f, 0.f, 0.f, 0.f);
const Rect Rect::one(0.f, 0.f, 1.f, 1.f);

//---------------------------------------------------------------------
Vec2::Vec2(float x, float y) : x(x), y(y)
{
}

//---------------------------------------------------------------------
Vec2 Vec2::unit(float angle)
{
	return Vec2(std::cosf(angle), std::sinf(angle));
}

//---------------------------------------------------------------------
float Vec2::length() const
{
	return std::sqrtf(x * x + y * y);
}

//---------------------------------------------------------------------
float Vec2::lengthSquared() const
{
	return x * x + y * y;
}

//---------------------------------------------------------------------
void Vec2::clampLength(float f)
{
	float len = length();
	if (len > f) *this *= f / len;
}

//---------------------------------------------------------------------
Vec2 Vec2::normalize(Vec2 v)
{
	return (v.x || v.y) ? v /= v.length() : zero;
}

//---------------------------------------------------------------------
Vec2 &Vec2::operator+=(Vec2 const &v)
{
	x += v.x;
	y += v.y;
	return *this;
}

//---------------------------------------------------------------------
Vec2 &Vec2::operator-=(Vec2 const &v)
{
	x -= v.x;
	y -= v.y;
	return *this;
}

//---------------------------------------------------------------------
Vec2 &Vec2::operator/=(float s)
{
	x /= s;
	y /= s;
	return *this;
}

//---------------------------------------------------------------------
Vec2 &Vec2::operator*=(float s)
{
	x *= s;
	y *= s;
	return *this;
}

//---------------------------------------------------------------------
Vec2 Vec2::operator-() const
{
	return Vec2(-x, -y);
}

//---------------------------------------------------------------------
Rect::Rect(float x, float y, float w, float h)
{
	top = y;
	left = x;
	right = x + w;
	bottom = y + h;
}

//---------------------------------------------------------------------
Rect::Rect(const Vec2 &center, float w, float h)
{
	top = center.y - h / 2;
	left = center.x - w / 2;
	right = left + w;
	bottom = top + h;
}

//---------------------------------------------------------------------
Rect::Rect(const Vec2 &tl, const Vec2 &br)
{
	top = tl.y;
	left = tl.x;
	right = br.x;
	bottom = br.y;
}

//---------------------------------------------------------------------
float Rect::width() const
{
	return right - left;
}

//---------------------------------------------------------------------
float Rect::height() const
{
	return bottom - top;
}

//---------------------------------------------------------------------
Vec2 Rect::size() const
{
	return Vec2(right - left, bottom - top);
}

//---------------------------------------------------------------------
Vec2 Rect::center() const
{
	return Vec2(right + left, bottom + top) / 2.f;
}

//---------------------------------------------------------------------
Curve::Curve(const Vec2 &a, const Vec2 &b, float h) : a(a), b(b), height(h)
{
}

//--------------------------------------------------------------------
Vec2 Curve::evaluate(float t, float h)
{
	// calculate base vectors
	Vec2 v = Vec2::normalize(b - a);
	Vec2 n(-v.y, v.x);

	// distance between endpoints
	float length = (b - a).length();

	// evaluate r(t)
	Vec2 r(t * length, height * std::sin(t * Pi));

	// evaluate r'(t) and rotate 90 degrees
	Vec2 dr(-height * Pi * std::cos(t  * Pi), length);

	// height
	dr = Vec2::normalize(dr) * h;

	// sum components
	return a + (v * r.x + n * r.y) + (v * dr.x + n * dr.y);
}

//--------------------------------------------------------------------
float Curve::tangent(float t)
{
	// calculate base vectors
	Vec2 v = Vec2::normalize(b - a);
	Vec2 n(-v.y, v.x);

	// evaluate r'(t)
	Vec2 dr((b - a).length(), height * Pi * std::cos(t  * Pi));

	// project
	dr = v * dr.x + n * dr.y;

	// calculate tangent's angle
	return std::atan2f(dr.y, dr.x);
}
