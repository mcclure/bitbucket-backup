#pragma once
#include <vector>
#include <stdint.h>
#include "constants.h"

#define RED 0
#define GREEN 1
#define BLUE 2

#define MAX_AGE 256
struct Pixel {
	float color[3];
	union {
		uint32_t age;
	};
	Pixel(float _r = 1, float _g = 1, float _b = 1) { color[0] = _r; color[1] = _g; color[2] = _b; age = MAX_AGE; }
	Pixel randomNear(float _tolerance = 0.1) const;
};
typedef Pixel** board;
extern board state;
extern board stateScratch;
extern board colorLookup; // special case, currently used only by the lighting postprocess for the background image
extern int worldWidth, worldHeight;
board makeBoard();
inline void swapBoard() { board temp = state; state = stateScratch; stateScratch = temp; }

struct GroupTuning
{
	bool applySpringAttachmentForces;
	float sameGroupRepulsionDistance;
	float otherGroupRepulsionDistance;
	float sameGroupRepulsionAmount;
	float otherGroupRepulsionAmount;
	float attractionDistance;
	float sameGroupAttractionAmount;
	float otherGroupAttractionAmount;
	GroupTuning(bool _applySpringAttachmentForces  = true, float _sameGroupRepulsionDistance = 4, float _otherGroupRepulsionDistance = 5, float _sameGroupRepulsionAmount = 0.002, float _otherGroupRepulsionAmount = 0.001, float _attractionDistance = 20.0, float _sameGroupAttractionAmount = 0.01, float _otherGroupAttractionAmount = 0.0) :
		applySpringAttachmentForces(_applySpringAttachmentForces), sameGroupRepulsionDistance(_sameGroupRepulsionDistance), otherGroupRepulsionDistance(_otherGroupRepulsionDistance), sameGroupRepulsionAmount(_sameGroupRepulsionAmount), otherGroupRepulsionAmount(_otherGroupRepulsionAmount), attractionDistance(_attractionDistance), sameGroupAttractionAmount(_sameGroupAttractionAmount), otherGroupAttractionAmount(_otherGroupAttractionAmount) {}
};
extern GroupTuning* groupTuning;

/*
+------------------------------------------------------------------------------+
|								   Globals									   |
+------------------------------------------------------------------------------+

This contains objects such as the grid of cells which we use all over. It has no
external dependencies except for constants.h. It should remain safe to include 
anywhere.
*/


extern board state;			// the state of each grid cell
void setupBoards();
void teardownBoards();

