#pragma once
#include <iostream>
#include <SFML/Graphics.hpp>
#include "constants.h"
#include "Particle.h"
#include "Emitter.h"
#include "globals.h"


/*
+------------------------------------------------------------------------------+
|							 Global Graphics Objects						   |
+------------------------------------------------------------------------------+

This contains only variables and objects for drawing to the screen. It has no
external dependencies except for constants.h. It should remain safe to include 
anywhere.
*/


extern sf::Uint8* pixel; // contains r, g, and b data in a one dimensional list
// sprite object which is drawn to the window (this is the only object- 
// everything is done as if it's raster based
extern sf::Sprite sprite; 
extern sf::Texture* texture;
extern sf::RenderWindow* window;	// game window

extern bool fullscreen;			// true if the game is running full screen
extern uint zoom;				// pixels are rendered this many pixels across
extern uint renderWidth;	
// the width, pre-zoom, of the render area
extern uint renderHeight;		// same (height)
extern uint worldDrawSize;		// size of rendered portion of world

extern std::vector<Particle*> particle;
extern std::vector<Emitter*> emitter;
struct Bin
{
	float left, top, right, bottom;
	int memberParticleCount;
	std::vector<Particle*> memberParticle;
	Bin() : memberParticleCount(1) {};
	void setSize(float _left, float _top, float _width, float _height) { left = _left; top = _top; right = _left + _width; bottom = _top + _height; }
};

void setupGraphicsObjects();
void teardownGraphicsObjects();
void connectSprings();
void oneWaySpringLink(Particle* particle1, Particle* particle2);
Particle* spawnParticle(float x, float y, int group);
void setupWindow(bool fullscreen);
void resetBins();
void sortIntoBin(Particle* currentParticle);
