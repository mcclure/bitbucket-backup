/*
 *  sound.h
 *  Jumpman
 *
 *  Copyright 2008 Andi McClure. All rights reserved.
 *
 */

// Very simple audio playback objects (square tone, noise channel, sample)

#ifndef JUMP_SOUND_H
#define JUMP_SOUND_H

struct noise {
	int mask;
    int ticks;
	int max;
    virtual void to(double &tosample) = 0;
	virtual void boundary(int) {} // Argument is slen
    inline bool done() { return ticks <= 0; } // TODO: Make virtual or something
	virtual noise *clone(unsigned int a) = 0;
}; // TODO: Something?

// EXCUSES: The sound classes contain much code duplication because I want 'inline' to work.

// SOUNDS

struct sqtone : public noise {
	int w;
	double amp;
	sqtone(int _mask, int _max, int _w, double _amp) : amp(_amp), w(_w) {
		mask = _mask;
        max = _max;
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
	inline void to(double &tosample) { double s = sample(); tosample += s; }
	virtual noise *clone(unsigned int a) { return new sqtone(a|mask, max, w, amp); }
};

extern int tapping; // Ugh don't... don't do this

struct slidetone : public noise {
	int w, flipnext, flipat, flips;
	double amp;
	slidetone(int _mask, int _max, int _w, double _amp) : amp(_amp), w(_w) {
		mask = _mask;
        max = _max;
		ticks = max;
        flipnext = 0;
        flipat = -1;
        flips = 0;
	}
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
            if (w && tapping >= flipnext) {
				value += (flips%2?-1:1)
					* amp;
                flipnext = tapping + w;
				flips++;
            }
		}
		return value;
	}
	inline void to(double &tosample) { double s = sample(); tosample += s; }
	virtual noise *clone(unsigned int a) { return new slidetone(a|mask, max, w, amp); }
};

struct notone : public noise {
	int nextstepat;
	int w;
	double amp;
	double stair;
	notone(int _mask, int _max, int _w, double _amp) : amp(_amp), w(_w), stair(0) {
		mask = _mask;
        max = _max;
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
	inline void to(double &tosample) { double s = sample(); tosample += s; }
	virtual noise *clone(unsigned int a) { return new notone(a|mask, max, w, amp); }
};

struct sampletone : public noise {
    double *source;
    sampletone(int _mask, double *_source, int len) : source(_source) {
		mask = _mask;
        max = len;
        ticks = max;
    }
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
            value = source[max-ticks];
			ticks--;
		}
		return value;
	}
	inline void to(double &tosample) { double s = sample(); tosample += s; }
	virtual noise *clone(unsigned int a) { return new sampletone(a|mask, source, max); }
};

#define DSTHRESH 0.000500
struct debugsampletone : public noise {
    double *source;
	int len;
	bool on;
	double lasthigh;
    debugsampletone(int _mask, double *_source, int _len) : source(_source), len(_len) {
		mask = _mask;
        max = len;
        ticks = max;
		lasthigh = 0;
		on = true;
    }
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
            value = source[max-ticks];
			ticks--;
		}
		if (0 == ticks) {
			ticks = max;
		}
		if (!on) {
			if (fabs(value) > DSTHRESH) {
				on = true;
			} else {
				value = 0;
			}
		}
		double fv = fabs(value);
		if (fv > lasthigh) lasthigh = fv;
		return value;
	}
	inline void to(double &tosample) { double s = sample(); tosample += s; }
	virtual void boundary(int) {
		ERR("LASTHIGH %lf\n", lasthigh);
		lasthigh = 0;
		if (lasthigh < DSTHRESH) {
			on = false;
		}
	} // Argument is slen
	virtual noise *clone(unsigned int a) { return new debugsampletone(a|mask, source, len); }
};

#define FLUTTERSIN 0
#define ONLYONCE 1

struct fluttertone : public noise {
	int w;
	double amp, f;
	int t2, period;
	static bool bd;
	fluttertone(int _mask, int _max, double _f, double _amp) : amp(_amp) {
		f = _f;
		w = 100 * f + 1;
		mask = _mask;
        max = _max;
		ticks = max;
		period = 0;
	}
	inline bool perioding() {
		return !period 
#if !ONLYONCE
		|| !(bd || (t2/period)%4)
#endif
		;
	}
	inline double sample() {
		double value = 0;
		if (perioding()) {
			value += 
#if FLUTTERSIN
			sin(t2 * (2*M_PI) * f / 44100.0)
#else
			((t2) % (w*2) >= w ? 1 : -1)
#endif
			* amp;
		}
		t2++;
		return value;
	}
	inline void to(double &tosample) { double s = sample(); tosample += s; }
	virtual noise *clone(unsigned int a) { return new fluttertone(a|mask, max, w, amp); }
};


#endif // JUMP_SOUND_H