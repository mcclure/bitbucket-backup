#pragma once

#include <cmath>

namespace ld
{
	// The pi constant.
	const float Pi = 3.14159265f;

	// The tau constant.
	const float Tau = 6.28318531f;

	//
	// 2D Vector.
	//
	struct Vec2
	{
		//
		// Default constructor.
		//
		explicit Vec2(float x = 0, float y = 0);

		//
		// Gets the vector <1, 0> rotated the given angle.
		//
		static Vec2 unit(float);

		//
		// Gets the length of this vector.
		//
		float length() const;

		//
		// Gets the squared length of this vector.
		//
		float lengthSquared() const;

		//
		// Clamps the length of this vector.
		//
		void clampLength(float);

		//
		// Returns a given vector normalized.
		//
		static Vec2 normalize(Vec2);

		//
		// Addition assignment operator.
		//
		Vec2 &operator+=(const Vec2&);

		//
		// Subtraction assignment operator.
		//
		Vec2 &operator-=(const Vec2&);

		//
		// Division assignment operator.
		//
		Vec2 &operator/=(float);

		//
		// Multiplication assignment operator.
		//
		Vec2 &operator*=(float);

		//
		// Negation operator.
		//
		Vec2 operator-() const;

		// The <0, 0> vector.
		static const Vec2 zero;

		// The <1, 1> vector.
		static const Vec2 one;

		// The x coordinate.
		float x;

		// The y coordinate.
		float y;
	};

	//
	// 2D Rectangle.
	//
	struct Rect
	{
		//
		// Constructor.
		//
		Rect(float x = 0, float y = 0, float width = 0, float height = 0);

		//
		// Constructor.
		//
		Rect(const Vec2 &center, float width, float height);

		//
		// Constructor.
		//
		Rect(const Vec2 &topleft, const Vec2 &bottomright);

		//
		// Gets the width.
		//
		float width() const;

		//
		// Gets the height.
		//
		float height() const;

		//
		// Gets the size.
		//
		Vec2 size() const;

		//
		// Gets the center of the rectangle.
		//
		Vec2 center() const;

		// The <0, 0, 0, 0> rectangle.
		static const Rect zero;

		// The <0, 0, 1, 1> rectangle.
		static const Rect one;

		// The top-left corner x coordinate.
		float left;

		// The top-left corner y coordinate.
		float top;

		// The botom-right corner x coordinate.
		float right;

		// The botom-right corner y coordinate.
		float bottom;
	};

	//
	// Represents a curve in 2D.
	//
	struct Curve
	{
		//
		// Constructor.
		//
		Curve(const Vec2& = Vec2::zero, const Vec2& = Vec2::zero, float = 0.f);

		//
		// Gets a point perpendicular to the slope at the specified instant.
		//
		Vec2 evaluate(float t, float h = 0.f);

		//
		// Gets the angle of the slope at the specified instant.
		//
		float tangent(float t);

		// The endpoints.
		Vec2 a, b;

		// The curve's height.
		float height;
	};

	//---------------------------------------------------------------------
	inline Vec2 operator+(Vec2 lhs, Vec2 const &rhs)
	{
		return lhs += rhs;
	}

	//---------------------------------------------------------------------
	inline Vec2 operator-(Vec2 lhs, Vec2 const &rhs)
	{
		return lhs -= rhs;
	}

	//---------------------------------------------------------------------
	inline Vec2 operator/(Vec2 lhs, float rhs)
	{
		return lhs /= rhs;
	}

	//---------------------------------------------------------------------
	inline Vec2 operator*(Vec2 lhs, float rhs)
	{
		return lhs *= rhs;
	}

	//---------------------------------------------------------------------
	inline bool operator==(const Vec2 &a, const Vec2 &b)
	{
		return a.x == b.x && a.y == b.y;
	}

	//---------------------------------------------------------------------
	inline bool operator!=(const Vec2 &a, const Vec2 &b)
	{
		return a.x != b.x || a.y != b.y;
	}

	//---------------------------------------------------------------------
	inline bool operator==(const Rect &a, const Rect &b)
	{
		return a.right == b.right && a.bottom == b.bottom && a.left == b.left && a.top == b.top;
	}

	//---------------------------------------------------------------------
	inline bool operator!=(const Rect &a, const Rect &b)
	{
		return a.right != b.right || a.bottom != b.bottom || a.left != b.left || a.top != b.top;
	}
}
