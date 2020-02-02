#pragma once

#include "Metro.h"
#include "Particles.h"

//
// Un voyageur dans le métro.
//
class Voyageur
{
public:
	//
	// Constructor.
	//
	Voyageur();	

	//
	// Gets the current station or the destination if between stations.
	//
	Station *getStation() const;

	//
	// Gets the current position.
	//
	ld::Vec2 getPosition() const;

	//
	// Gets the target direction.
	//
	Direction getDirection() const;

	//
	// Query if this voyageur is currently moving.
	//
	bool isMoving() const;

	//
	// Places this voyageur at a given station.
	//
	void placeAt(Station*);

	//
	// Stop at the next station.
	//
	void stop();

	//
	// Takes the next train. Returns false if there's no connection.
	//
	bool move(const Direction&);

	//
	// Updates this object.
	//
	virtual void update();

private:
	// The next stop.
	Station *nextStop;

	// Current movement.
	Stretch *movement;

	// Target direction.
	Direction direction;

	// Is inside a train?
	bool insideTrain;
};

//
// The player.
//
class Player : public Voyageur
{
public:
	//
	// Default Constructor.
	//
	Player();

	//
	// Updates this object.
	//
	void update();

	//
	// Draws this object.
	//
	void draw();

private:
	// Particle emitters.
	Particles emitters[2];
};

//
// The police.
//
class Cop : public Voyageur
{
public:
	//
	// Default Constructor.
	//
	Cop(const Direction&, Station*);

	//
	// Updates this object.
	//
	void update();

private:
	// Patrol direction.
	Direction patrol;

	// Patrol intervals.
	float cooldown;
};
