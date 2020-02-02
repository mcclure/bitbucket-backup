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

class VEC3
{
public:
    VEC3() {}
    VEC3(float x, float y, float z) { vec[0] = x; vec[1] = y; vec[2] = z; }
    VEC3(float x) { vec[0] = x; vec[1] = x; vec[2] = x; }
    VEC3(const VEC3 &v) { vec[0] = v.vec[0]; vec[1] = v.vec[1]; vec[2] = v.vec[2]; }

    VEC3	&operator=(float f)
    { *this = VEC3(f); return *this; }
    VEC3	&operator=(const VEC3 &v)
    { vec[0] = v.vec[0]; vec[1] = v.vec[1]; vec[2] = v.vec[2]; return *this; }

    float	x() const { return vec[0]; }
    float	&x() { return vec[0]; }
    float	y() const { return vec[1]; }
    float	&y() { return vec[1]; }
    float	z() const { return vec[2]; }
    float	&z() { return vec[2]; }

    // For when it is colour.
    float	r() const { return vec[0]; }
    float	&r() { return vec[0]; }
    float	g() const { return vec[1]; }
    float	&g() { return vec[1]; }
    float	b() const { return vec[2]; }
    float	&b() { return vec[2]; }

    void	normalize()
    { float len = length(); if (len > 0.00001) { vec[0] /= len; vec[1] /= len;  vec[2] /= len; } }
    float	length() const
    { return sqrt(length2()); }
    float	length2() const
    { return vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]; }


    float	operator()(int i) const { return vec[i]; }
    float	&operator()(int i) { return vec[i]; }

    bool	 operator==(const VEC3 &v)
    { return vec[0] == v.vec[0] && vec[1] == v.vec[1] && vec[2] == v.vec[2]; }

    VEC3	&operator+=(const VEC3 &v)
    { vec[0] += v.vec[0]; vec[1] += v.vec[1]; vec[2] += v.vec[2]; return *this;}
    VEC3	&operator*=(const VEC3 &v)
    { vec[0] *= v.vec[0]; vec[1] *= v.vec[1]; vec[2] *= v.vec[2]; return *this;}
    VEC3	&operator-=(const VEC3 &v)
    { vec[0] -= v.vec[0]; vec[1] -= v.vec[1]; vec[2] -= v.vec[2]; return *this;}
    VEC3	&operator/=(const VEC3 &v)
    { vec[0] /= v.vec[0]; vec[1] /= v.vec[1]; vec[2] /= v.vec[2]; return *this;}

    VEC3	&operator*=(float v)
    { vec[0] *= v; vec[1] *= v; vec[2] *= v; return *this; }
    VEC3	&operator/=(float v)
    { vec[0] /= v; vec[1] /= v; vec[2] /= v; return *this; }

    VEC3	 operator-() const
    { return VEC3(-vec[0], -vec[1], -vec[2]); }

    void	clamp(const VEC3 &vmin, const VEC3 &vmax)
    {
	vec[0] = BOUND(vec[0], vmin.vec[0], vmax.vec[0]);
	vec[1] = BOUND(vec[1], vmin.vec[1], vmax.vec[1]);
	vec[2] = BOUND(vec[2], vmin.vec[2], vmax.vec[2]);
    }

private:
    float		vec[3];
};

inline VEC3 operator+(const VEC3 &v1, const VEC3 &v2)
{ return VEC3(v1.x()+v2.x(), v1.y()+v2.y(), v1.z()+v2.z()); }
inline VEC3 operator-(const VEC3 &v1, const VEC3 &v2)
{ return VEC3(v1.x()-v2.x(), v1.y()-v2.y(), v1.z()-v2.z()); }
inline VEC3 operator*(const VEC3 &v1, const VEC3 &v2)
{ return VEC3(v1.x()*v2.x(), v1.y()*v2.y(), v1.z()*v2.z()); }
inline VEC3 operator/(const VEC3 &v1, const VEC3 &v2)
{ return VEC3(v1.x()/v2.x(), v1.y()/v2.y(), v1.z()/v2.z()); }

inline VEC3 operator*(float v1, const VEC3 &v2)
{ return VEC3(v1*v2.x(), v1*v2.y(), v1*v2.z()); }
inline VEC3 operator/(float v1, const VEC3 &v2)
{ return VEC3(v1/v2.x(), v1/v2.y(), v1/v2.z()); }
inline VEC3 operator*(const VEC3 &v1, float v2)
{ return VEC3(v1.x()*v2, v1.y()*v2, v1.z()*v2); }
inline VEC3 operator/(const VEC3 &v1, float v2)
{ return VEC3(v1.x()/v2, v1.y()/v2, v1.z()/v2); }

inline float dist(VEC3 v1, VEC3 v2)
{ return (v1 - v2).length(); }
inline float quadrature(VEC3 v1, VEC3 v2)
{ return (v1 - v2).length2(); }

