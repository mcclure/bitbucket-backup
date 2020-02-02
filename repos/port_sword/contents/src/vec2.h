/*
 * Licensed under Tridude Heart license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Tridude Heart Development
 *
 * NAME:        sprite.cpp ( Tridude, C++ )
 *
 * COMMENTS:
 */

#pragma once

#include "rand.h"
#include <math.h>

class VEC2
{
public:
    VEC2() {}
    VEC2(float x, float y) { vec[0] = x; vec[1] = y; }
    VEC2(float x) { vec[0] = x; vec[1] = x; }
    VEC2(const VEC2 &v) { vec[0] = v.vec[0]; vec[1] = v.vec[1]; }

    VEC2	&operator=(float f)
    { *this = VEC2(f); return *this; }
    VEC2	&operator=(const VEC2 &v)
    { vec[0] = v.vec[0]; vec[1] = v.vec[1]; return *this; }

    float	x() const { return vec[0]; }
    float	&x() { return vec[0]; }
    float	y() const { return vec[1]; }
    float	&y() { return vec[1]; }

    void	normalize()
    { float len = length(); if (len > 0.00001) { vec[0] /= len; vec[1] /= len; } }
    float	length() const
    { return sqrt(length2()); }
    float	length2() const
    { return vec[0]*vec[0] + vec[1]*vec[1]; }


    float	operator()(int i) const { return vec[i]; }
    float	&operator()(int i) { return vec[i]; }

    bool	 operator==(const VEC2 &v)
    { return vec[0] == v.vec[0] && vec[1] == v.vec[1]; }

    VEC2	&operator+=(const VEC2 &v)
    { vec[0] += v.vec[0]; vec[1] += v.vec[1]; return *this; }
    VEC2	&operator*=(const VEC2 &v)
    { vec[0] *= v.vec[0]; vec[1] *= v.vec[1]; return *this; }
    VEC2	&operator-=(const VEC2 &v)
    { vec[0] -= v.vec[0]; vec[1] -= v.vec[1]; return *this; }
    VEC2	&operator/=(const VEC2 &v)
    { vec[0] /= v.vec[0]; vec[1] /= v.vec[1]; return *this; }

    VEC2	&operator*=(float v)
    { vec[0] *= v; vec[1] *= v; return *this; }
    VEC2	&operator/=(float v)
    { vec[0] /= v; vec[1] /= v; return *this; }

    VEC2	 operator-() const
    { return VEC2(-vec[0], -vec[1]); }

    void	clamp(const VEC2 &vmin, const VEC2 &vmax)
    {
	vec[0] = BOUND(vec[0], vmin.vec[0], vmax.vec[0]);
	vec[1] = BOUND(vec[1], vmin.vec[1], vmax.vec[1]);
    }

    void	rotate(float angle)
    {
	float		c, s, nx, ny;
	c = cos(angle);
	s = sin(angle);

	nx = c * x() - s * y();
	ny = s * x() + c * y();
	vec[0] = nx;
	vec[1] = ny;
    }
    void	rotate90()
    {
	float		tx = x();
	vec[0] = -vec[1];
	vec[1] = tx;
    }

private:
    float		vec[2];
};

inline VEC2 operator+(const VEC2 &v1, const VEC2 &v2)
{ return VEC2(v1.x()+v2.x(), v1.y()+v2.y()); }
inline VEC2 operator-(const VEC2 &v1, const VEC2 &v2)
{ return VEC2(v1.x()-v2.x(), v1.y()-v2.y()); }
inline VEC2 operator*(const VEC2 &v1, const VEC2 &v2)
{ return VEC2(v1.x()*v2.x(), v1.y()*v2.y()); }
inline VEC2 operator/(const VEC2 &v1, const VEC2 &v2)
{ return VEC2(v1.x()/v2.x(), v1.y()/v2.y()); }

inline VEC2 operator*(float v1, const VEC2 &v2)
{ return VEC2(v1*v2.x(), v1*v2.y()); }
inline VEC2 operator/(float v1, const VEC2 &v2)
{ return VEC2(v1/v2.x(), v1/v2.y()); }
inline VEC2 operator*(const VEC2 &v1, float v2)
{ return VEC2(v1.x()*v2, v1.y()*v2); }
inline VEC2 operator/(const VEC2 &v1, float v2)
{ return VEC2(v1.x()/v2, v1.y()/v2); }

inline float dist(VEC2 v1, VEC2 v2)
{ return (v1 - v2).length(); }
inline float quadrature(VEC2 v1, VEC2 v2)
{ return (v1 - v2).length2(); }

