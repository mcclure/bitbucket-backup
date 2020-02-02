#pragma once
#include <SFML/Graphics.hpp>
#include "constants.h"
#include <iostream>

void setupAudio();
void updateAudio();
void teardownAudio();

// Scene

#define AUDIOPARAMS 8

extern float audioparam[AUDIOPARAMS];

void debug_audioparam_ptr(int dir);
void debug_audioparam_incr(int dir);