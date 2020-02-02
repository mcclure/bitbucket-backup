#pragma once

#include <vector>

#include "BaseGame.h"

//
// Draws basic shapes.
//
class Shapes
{
public:
	//
	// Default constructor.
	//
	Shapes();

	//
	// Sets the foreground color.
	//
	void setColor(const ld::Color&);

	//
	// Draws segments.
	//
	void drawSegment(const ld::Vec2&, const ld::Vec2&, float width);

	//
	// Draws segments.
	//
	void drawSegments(const std::vector<ld::Vec2>&, float width);

	//
	// Draws a polygon.
	//
	void drawPolygon(const std::vector<ld::Vec2>&);

	//
	// Draws a rectangle.
	//
	void drawRectangle(const ld::Rect&);

	//
	// Draws an arc.
	//
	void drawArc(const ld::Vec2&, float radius, float start = 0.f, float length = ld::Tau);

	//
	// Draws a curve.
	//
	void drawCurve(const ld::Curve&, float width);

private:
	// The current color.
	ld::Color color;
};
