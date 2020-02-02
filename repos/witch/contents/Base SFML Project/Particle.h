#pragma once
#include <vector>
#include "constants.h"
#include "globals.h"

class Particle
{
public:
	float x, y;
	float startX, startY;
	float velocityX, velocityY;
	int group;
	int binX, binY; // for partitioning
	Pixel p;
	bool applyStartPositionSpring;
	bool applySpringAttachmentForces;
	bool applySpringAttachToAnchored;
	std::vector<Particle*> springAttachment;
	Particle(float x, float y, int group, Pixel p = Pixel());
	~Particle(void);
};

