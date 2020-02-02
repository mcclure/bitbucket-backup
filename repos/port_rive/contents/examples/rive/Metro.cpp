
#include <glfw.h>
#include <set>

#include "Metro.h"
#include "Shapes.h"

using namespace ld;

// Static initializations
float Metro::timeFactor = 1.f;

//---------------------------------------------------------------------
Stretch::Stretch(Station *a, Station *b, Ligne *ligne, float c)
	: a(a), b(b), ligne(ligne), curvature(c)
{
}

//---------------------------------------------------------------------
Curve Stretch::getCurve() const
{
	return Curve(a->position, b->position, curvature);
}

//---------------------------------------------------------------------
Station::Station(const Vec2 &pos, const std::wstring &name)
	: name(name), position(pos)
{
}

//---------------------------------------------------------------------
Stretch *Station::stretchTowards(const Direction &direction) const
{
	for (auto stretch : stretches)
	{
		if (stretch->ligne == direction.ligne && (stretch->a == this) == direction.forward)
			return stretch;
	}

	return false;
}

//---------------------------------------------------------------------
Ligne::Ligne(const Color &color, bool express) : color(color), isExpress(express)
{
	timer = random::unif();
}

//---------------------------------------------------------------------
float Ligne::trainPosition() const
{
	if (isExpress) return timer;
	return std::min(1.f, 2.f * timer);
}

//---------------------------------------------------------------------
float Ligne::trainCountdown() const
{
	if (isExpress) return 0.f;
	return trainsEnRoute() ? 0.f : (1.f - timer) * 2.f;
}

//---------------------------------------------------------------------
bool Ligne::trainsEnRoute() const
{
	if (isExpress) return timer + timeStep / Metro::timeFactor < 1.f;
	return timer < 0.5f;
}

//---------------------------------------------------------------------
void Ligne::forEachStation(std::function<bool(Station*)> function)
{
	if (stretches.empty() || !function(stretches.front().a))
		return;

	for (auto &stretch : stretches)
	{
		if (!function(stretch.b))
			return;
	}
}

//---------------------------------------------------------------------
void Ligne::update()
{
	timer += (isExpress ? 1.f : 0.5f) * timeStep / Metro::timeFactor;
	timer -= (int)timer;
}

//---------------------------------------------------------------------
Direction::Direction(Ligne *ligne, bool forward) : ligne(ligne), forward(forward)
{
}

//---------------------------------------------------------------------
Metro::~Metro()
{
	// delete stations
	for (auto &pair : stations)
		delete pair.second;
}

//---------------------------------------------------------------------
void Metro::update()
{
	// lignes
	for (auto &ligne : lignes)
		ligne.update();
}

//---------------------------------------------------------------------
void Metro::draw()
{
	Shapes shapes;

	// for each ligne
	for (auto &ligne : lignes)
	{
		// for each connection
		for (auto &path : ligne.stretches)
		{
			Curve curve = path.getCurve();

			// background
			if (!ligne.isExpress)
			{
				Color color = ligne.color;
				color.a = 0.4f;

				shapes.setColor(color);
				shapes.drawCurve(curve, 4.f);
			}

			// foreground
			shapes.setColor(ligne.color);
			shapes.drawCurve(curve, ligne.isExpress ? 2.f : 4.f * (1.f - ligne.trainCountdown()));
		}
	}

	// for each station
	for (auto &pair : stations)
	{
		Station &station = *pair.second;
		
		float arc = Tau / station.colors.size();
		unsigned i = 0;

		// for each connection
		for (auto &color : station.colors)
		{
			// draw arc
			shapes.setColor(color);
			shapes.drawArc(station.position, 5.f, arc * i++, arc);
		}
	}

	// draw cars
	shapes.setColor(Color(1.f, 1.f, 1.f, 0.5f));

	// for each ligne
	for (auto &ligne : lignes)
	{
		if (ligne.isExpress) continue;
		float instant = ligne.trainPosition();

		// for each connection
		for (auto &path : ligne.stretches)
		{
			Curve curve = path.getCurve();

			// forward
			matrix::push();
			matrix::mult(curve.evaluate(instant, 1.f), curve.tangent(instant));

			shapes.drawRectangle(Rect(Vec2::zero, 3.f, 2.f));

			matrix::pop();

			// backward
			matrix::push();
			matrix::mult(curve.evaluate(1.f - instant, -1.f), curve.tangent(1.f - instant));

			shapes.drawRectangle(Rect(Vec2::zero, 3.f, 2.f));

			matrix::pop();
		}
	}
}

//---------------------------------------------------------------------
Station *Metro::getStation(const std::wstring &name) const
{
	auto it = stations.find(name);
	return it == stations.end() ? nullptr : it->second;
}

//---------------------------------------------------------------------
Station *Metro::getRandomStation() const
{
	auto it = stations.begin();
	std::advance(it, (unsigned)(stations.size() * random::unif()));
    return it->second;
}

//---------------------------------------------------------------------
void Metro::forEachLigne(std::function<bool(Ligne*)> function)
{
	for (auto &ligne : lignes)
	{
		if (!function(&ligne))
			return;
	}
}
