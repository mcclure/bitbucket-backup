#pragma once

#include <list>
#include <vector>
#include <map>

#include "BaseGame.h"

// Forward references.
struct Station;
struct Ligne;
struct Direction;

//
// Une connéxion de deux stations.
//
struct Stretch
{
	//
	// Constructor.
	//
	Stretch(Station* = nullptr, Station* = nullptr, Ligne* = nullptr, float = 0.f);

	//
	// Gets the curve that represents this object.
	//
	ld::Curve getCurve() const;

	// Les stations.
	Station *a, *b;

	// La ligne.
	Ligne *ligne;

	// The curvature.
	float curvature;
};

//
// Une station du métro.
//
struct Station
{
	//
	// Constructor.
	//
	Station(const ld::Vec2&, const std::wstring&);

	//
	// Gets the stretch towards a direction.
	//
	Stretch *stretchTowards(const Direction&) const;

	// The name.
	const std::wstring name;

	// The position.
	const ld::Vec2 position;

	// Les connéxions.
	std::list<Stretch*> stretches;

	// Les couleurs des lignes.
	std::vector<ld::Color> colors;
};

//
// Une ligne du métro.
//
struct Ligne
{
	//
	// Constructor.
	//
	Ligne(const ld::Color&, bool express = false);

	//
	// Gets the current position of all trains.
	//
	float trainPosition() const;

	//
	// Gets the relative instant of the trains' countdown. 0 if en route.
	//
	float trainCountdown() const;

	//
	// Query if the trains are en route.
	//
	bool trainsEnRoute() const;

	//
	// Calls a function for each station. Return false to stop.
	//
	void forEachStation(std::function<bool(Station*)>);

	//
	// Updates this object.
	//
	void update();

	// The color.
	const ld::Color color;

	// Is it an express line?
	const bool isExpress;

	// Les connéxions sur cette ligne.
	std::list<Stretch> stretches;
	
private:
	// The internal timer
	float timer;
};

//
// Une diréction dans le métro.
//
struct Direction
{
	//
	// Constructor.
	//
	Direction(Ligne* = nullptr, bool = true);

	//
	// Is this direction valid?
	//
	bool isValid() const;

	// La ligne.
	Ligne *ligne;

	// Going forward?
	bool forward;
};

//
// Manages the Metro.
//
class Metro
{
public:
	//
	// Default constructor.
	//
	Metro();

	//
	// Destructor.
	//
	~Metro();

	//
	// Updates the game.
	//
	void update();

	//
	// Draws the game.
	//
	void draw();

	//
	// Gets a station by its name.
	//
	Station *getStation(const std::wstring&) const;

	//
	// Gets a random station.
	//
	Station *getRandomStation() const;

	//
	// Calls a function for each ligne. Return false to stop.
	//
	void forEachLigne(std::function<bool(Ligne*)>);

	// Time factor.
	static float timeFactor;

private:
	// Stations.
	std::map<std::wstring, Station*> stations;

	// Lignes.
	std::list<Ligne> lignes;
};


//---------------------------------------------------------------------
inline bool Direction::isValid() const
{
	return ligne != nullptr;
}

//---------------------------------------------------------------------
inline bool operator==(const Direction &a, const Direction &b)
{
	return a.ligne == b.ligne && a.forward == b.forward;
}

//---------------------------------------------------------------------
inline bool operator!=(const Direction &a, const Direction &b)
{
	return a.ligne != b.ligne || a.forward != b.forward;
}
