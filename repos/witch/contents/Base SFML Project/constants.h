#pragma once
#include <stdlib.h>
#include <algorithm>
#include <string>
#include "math.h"

#define uint unsigned int

using namespace std;

#define RANDOM_MAX ((((unsigned int)1)<<31)-1)

#ifdef WINDOWS

#define srandom srand
inline long random() {
	unsigned long r1 = rand();
	unsigned long r2 = rand();
	return ((r1 << 16)|r2) & RANDOM_MAX;
}

#endif

#ifdef __APPLE__
#define RESOURCES mac_resources_path + "/resources/"
#else
#define RESOURCES "resources/"
#endif

extern string mac_resources_path;

#define FOREACH( var, vector ) for(int c = 0; c < vector.size(); c++) { var = vector[c];

inline float randomfloat(float min, float max) { 
	return float(random())/RANDOM_MAX * (max-min) + min; 
}

inline float clamp(float lo, float in, float hi) {
	return ::max( ::min(in, hi), lo );
}

inline float square(float a) { return a*a; }

inline float lerp(float blend, float value1, float value2) {
	return value1 * (1.0 - blend) + value2 * blend;
}