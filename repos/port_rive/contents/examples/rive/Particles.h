#pragma once

#include <list>

#include "BaseGame.h"

//
// Simple particle emitter.
//
struct Particles
{
	//
	// Default constructor.
	//
	Particles();

	//
	// Updates this emitter.
	//
	void update();

	//
	// Draws this emitter.
	//
	void draw();

	//
	// Clear all particles.
	//
	void reset();

	// The position.
	ld::Vec2 position;

	// The color of all particles.
	ld::Color color;

	// The direction of the emission.
	float direction;

	// The maximum half-arc of variation.
	float variation;

	// The velocity of all particles.
	float velocity;

	// The size of the particles.
	float size;

	// The duration of each particle. (in seconds)
	float duration;

	// The interval between emissions. (in seconds)
	float interval;

	// Spawns new particles.
	bool enabled;

private:
	//
	// A single particle.
	//
	struct Unit
	{
		// Parameters.
		ld::Vec2 position, velocity;

		// State.
		float life;
	};

	// The particles currently alive.
	std::list<Unit> units;

	// The cooldown between emissions.
	float cooldown;
};
