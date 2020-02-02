
#include <glfw.h>

#include "Shapes.h"

using namespace ld;

// Number of vertices in a circle
const unsigned CircleVertices = 30;

//---------------------------------------------------------------------
inline void glVec2(const Vec2 &v)
{
	glVertex2f(v.x, v.y);
}

//---------------------------------------------------------------------
inline void glColor(const Color &c)
{
	glColor4f(c.r, c.g, c.b, c.a);
}

//---------------------------------------------------------------------
Shapes::Shapes()
{
}

//---------------------------------------------------------------------
void Shapes::setColor(const Color &color)
{
	this->color = color;
}

//---------------------------------------------------------------------
void Shapes::drawSegment(const ld::Vec2 &a, const ld::Vec2 &b, float width)
{
	glColor(color);

	glBegin(GL_TRIANGLE_STRIP);
	{
		Vec2 v = Vec2::normalize(a - b) * width / 2.f;
		v = Vec2(v.y, -v.x);

		glVec2(a + v);
		glVec2(a - v);
		glVec2(b + v);
		glVec2(b - v);
	}
	glEnd();
}

//---------------------------------------------------------------------
void Shapes::drawSegments(const std::vector<ld::Vec2> &vertices, float width)
{
	if (vertices.size() < 2) return;
	glColor(color);

	glBegin(GL_TRIANGLE_STRIP);
	{
		// pairs of vertices
		for (unsigned i = 0; i < vertices.size() - 1; ++i)
		{
			Vec2 v = vertices[i + 1] - vertices[i];
			v = Vec2::normalize(v) * width / 2.f;

			v = Vec2(v.y, -v.x);

			glVec2(vertices[i] + v);
			glVec2(vertices[i] - v);
			glVec2(vertices[i + 1] + v);
			glVec2(vertices[i + 1] - v);
		}
	}
	glEnd();
}

//---------------------------------------------------------------------
void Shapes::drawPolygon(const std::vector<ld::Vec2> &vertices)
{
	glColor(color);

	glBegin(GL_TRIANGLE_FAN);
	{
		for (const Vec2 &v : vertices)
			glVec2(v);	
	}
	glEnd();
}

//---------------------------------------------------------------------
void Shapes::drawRectangle(const Rect &rect)
{
	glColor(color);

	glBegin(GL_TRIANGLE_STRIP);
	{
		glVec2(Vec2(rect.left, rect.top));
		glVec2(Vec2(rect.left, rect.bottom));
		glVec2(Vec2(rect.right, rect.top));
		glVec2(Vec2(rect.right, rect.bottom));
	}
	glEnd();
}

//---------------------------------------------------------------------
void Shapes::drawArc(const Vec2 &center, float radius, float start, float length)
{
	glColor(color);

	unsigned vertexCount = (unsigned)(CircleVertices * length / Tau);

	glBegin(GL_TRIANGLE_FAN);
	{
		glVec2(center);

		for (unsigned i = 0; i <= vertexCount; ++i)
			glVec2(center + Vec2::unit(start + i * Tau / CircleVertices) * radius);
	}
	glEnd();
}

//---------------------------------------------------------------------
void Shapes::drawCurve(const Curve &curve, float width)
{
	glColor(color);

	// calculate base vectors
	Vec2 v = Vec2::normalize(curve.b - curve.a);
	Vec2 n(-v.y, v.x);

	float length = (curve.b - curve.a).length();

	glBegin(GL_TRIANGLE_STRIP);
	{
		for (unsigned i = 0; i <= 16; ++i)
		{
			float t = (float)i / 16;

			Vec2 r(t * length, curve.height * std::sin(t * Pi));
			Vec2 dr(-curve.height * Pi * std::cos(t  * Pi), length);
			dr = Vec2::normalize(dr) * width / 2.f;

			glVec2(curve.a + (v * r.x + n * r.y) + (v * dr.x + n * dr.y));
			glVec2(curve.a + (v * r.x + n * r.y) - (v * dr.x + n * dr.y));
		}
	}
	glEnd();
}
