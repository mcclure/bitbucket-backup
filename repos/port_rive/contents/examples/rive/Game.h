#pragma once

#include <list>

#include "BaseGame.h"
#include "Metro.h"
#include "Voyageur.h"
#include "Shapes.h"

//
// Manages the internals of a game.
//
class Game : public ld::BaseGame
{
public:
	//
	// Default constructor.
	//
	Game();

	//
	// Destructor.
	//
	~Game();

	//
	// Restart the game.
	//
	void restart();

	//
	// Query if the this game has ended.
	//
	bool finished() const;

private:
	//
	// Updates the game.
	//
	void update();

	//
	// Draws the game.
	//
	void draw();

	//
	// Prepares for a new wave.
	//
	void prepareWave();

	// Le métro.
	Metro leMetro;

	// The player.
	Player *player;

	// All voyageurs.
	std::list<Voyageur*> voyageurs;

	// Game countdown.
	float countdown;

	// Arrested status.
	float arrested;

	// Bonus score (time)
	float bonusScore;

	// Score.
	unsigned score;

	// The camera position.
	ld::Vec2 camera;

	// The camera zoom.
	float cameraZoom;
	
	// Last station.
	Station *lastStation;

	// Station info effect.
	float stationNameTimer;

	// Money visual hint.
	Particles moneyPackage;

	// Timer used on effects.
	float effectTimer;

	// UI drawer.
	Shapes uiShapes;

	// Mouse button.
	bool receivingInput;

	// Fonts
	ld::Font bigFont, smallFont;

	// Sounds
	std::unique_ptr<ld::Sound[]> sfx;

	// Music
	ld::BackgroundMusic backgroundMusic, ambianceMusic;
};
