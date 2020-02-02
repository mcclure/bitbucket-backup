
#include "Particles.h"
#include "Shapes.h"

using namespace ld;

//---------------------------------------------------------------------
Particles::Particles() : enabled(true), cooldown(0.f)
{
	direction = 0.f;
	variation = 0.f;
	velocity = 1.f;
	size = 1.f;
	duration = 1.f;
	interval = 0.1f;
}

//---------------------------------------------------------------------
void Particles::update()
{
	// for each particle
	for (auto it = units.begin(); it != units.end();)
	{
		// move
		it->position += it->velocity * timeStep;

		// consume life
		it->life -= timeStep / duration;

		// die?
		it->life <= 0.f ? units.erase(it++) : ++it;
	}

	// spawn a particle?
	if (enabled && (cooldown -= timeStep / interval) <= 0.f)
	{
		// reset cooldown
		cooldown = 1.f;

		// spawn particle
		Unit unit = { position, Vec2::unit(direction + (random::unif() * 2.f - 1.f) * variation) * velocity, 1.f };
		units.push_back(unit);
	}
}

//---------------------------------------------------------------------
void Particles::draw()
{
	Color c = color;
	Shapes shapes;

	// for each particle
	for (auto &unit : units)
	{
		c.a = color.a * (1.f -  (1.f - unit.life) * (1.f - unit.life));
		shapes.setColor(c);

		shapes.drawRectangle(Rect(unit.position, size, size));
	}
}

//---------------------------------------------------------------------
void Particles::reset()
{
	units.clear();
}
