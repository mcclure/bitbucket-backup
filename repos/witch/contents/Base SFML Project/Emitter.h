#pragma once
#include "constants.h"

class Particle;
Particle* spawnParticle(float x, float y, int group);

class Emitter
{
public:
	float x, y;
	int group;
	int timer;
	Emitter(float x, float y, int group);
	~Emitter(void);
	void update();
	void trigger(int group);
};

