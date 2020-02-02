/*
 *  sound.h
 *  Jumpman
 *
 *  Copyright 2008 Andi McClure. All rights reserved.
 *
 */

#include <vector>
#include <queue>
#include <ext/hash_map>
using namespace std;
using namespace ::__gnu_cxx;

// Why here? Argh.
#define RANDOM_MAX ((1<<31)-1)
#ifdef WINDOWS
#define srandom srand
inline long random() {
	unsigned long r1 = rand();
	unsigned long r2 = rand();
	return ((r1 << 16)|r2) & RANDOM_MAX;
}
#endif

struct noise {}; // TODO: Something?

// EXCUSES: The sound classes contain much code duplication because I want 'inline' to work.

// SOUNDS

struct sqtone : noise {
	int ticks;
	int max, w;
	double amp;
	sqtone(int _max, int _w, double _amp) : max(_max), amp(_amp), w(_w), ticks(0) {}
	inline void reset() {
		ticks = max;
	}
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
			value += ((max - ticks) % (w*2) >= w ? 1 : -1)
						* ((double)(ticks)/max)
						* amp;
			ticks--;
		}
		return value;
	}
	inline void to(double &tosample) { tosample += sample(); }
};

struct notone : noise {
	int ticks, nextstepat;
	int max, w;
	double amp;
	double stair;
	notone(int _max, int _w, double _amp) : max(_max), amp(_amp), w(_w), ticks(0), stair(0) {}
	inline void reset() {
		ticks = max;
		nextstepat = ticks - w;
		stairstep();
	}
	inline void stairstep() {
		stair = (double)random()/RANDOM_MAX * 2 - 1;
	}
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
			if (ticks == nextstepat) {
				nextstepat = ticks - w;
				stairstep();
			}
			value += stair
						* ((double)(ticks)/max)
						* amp;
			ticks--;
		}
		return value;
	}
	inline void to(double &tosample) { tosample += sample(); }
};

// STATE

extern sqtone sjump;

extern sqtone sland;

extern sqtone sball;

extern sqtone sbell;

extern notone splodey;

extern notone slick;

extern notone shatter;

extern bool doingEnding;
extern int endingAt, endingAtMode, endingAtBar, endingAtMsg;
void endingTick(); void endingTickMainThread();

void soundInit();
