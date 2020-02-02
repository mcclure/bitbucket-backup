/*
 *  sound.h
 *  Jumpman
 *
 *  Copyright 2008 Andi McClure. All rights reserved.
 *
 */

#ifndef JUMP_SOUND_H
#define JUMP_SOUND_H

struct noise {
    int ticks;
	int max;
    virtual void to(double &tosample) = 0;
    inline bool done() { return ticks <= 0; } // TODO: Make virtual or something
}; // TODO: Something?

// EXCUSES: The sound classes contain much code duplication because I want 'inline' to work.

// SOUNDS

struct sqtone : public noise {
	int w;
	double amp;
	sqtone(int _max, int _w, double _amp) : amp(_amp), w(_w) {
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
	inline void to(double &tosample) { tosample += sample(); }
};

extern int tapping; // Ugh don't... don't do this

struct slidetone : public noise {
	int w, slideafter, slideon, slideby, flipnext, flipat, flips;
	double amp;
	slidetone(int _max, int _w, int _slideafter, int _slideon, int _slideby, double _amp) : amp(_amp), w(_w), slideafter(_slideafter), slideon(_slideon), slideby(_slideby) {
        max = _max;
		ticks = max;
        flipnext = 0;
        flipat = -1;
        flips = 0;
	}
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
            
            if (tapping >= flipnext) {
                flipat *= -1;
                flipnext = tapping + w;
                if (0 >= slideafter && 0 == flips++ % slideon)
                    w += slideby;
            }          
            slideafter--;
            
			value += flipat
            * ((double)(ticks)/max)
            * amp;

            if (0 >= slideafter)
                ticks--;
            else
                slideafter--;
		}
		return value;
	}
	inline void to(double &tosample) { tosample += sample(); }
};

struct notone : public noise {
	int nextstepat;
	int w;
	double amp;
	double stair;
	notone(int _max, int _w, double _amp) : amp(_amp), w(_w), stair(0) {
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
	inline void to(double &tosample) { tosample += sample(); }
};

struct sampletone : public noise {
    const double *source;
	double amp;
    sampletone(const double *_source, int len, double _amp = 1) : source(_source), amp(_amp) {
        max = len;
        ticks = max;
    }
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
            value = source[max-ticks]*amp;
			ticks--;
		}
		return value;
	}
    inline void to(double &tosample) { tosample += sample(); }
};

// Resample using linear interpolate/decimate
struct resampletone : public noise {
    const double *source;
	double amp;
	int c; // index within sample
	int i, d; // interpolate x, decimate x; "max" = source_len * interpolate x
    resampletone(const double *_source, int len, int _i, int _d, double _amp = 1) : source(_source), amp(_amp), c(0), i(_i), d(_d) {
        max = (len-1) * i; // Could make very short samples disappear completely?
		ticks = max>=0?1:0; // Will set to 0 manually later
    }
	inline double sample() {
		double value = 0;
		if (ticks > 0) {
			const double within = (c % i)/double(i);  // How far within interpolate "block"
			const int fromidx = c/i;     // index within source for first sample
			const int toidx = fromidx + 1; // index within source for second sample
			
			value = (source[fromidx]*(1-within) + source[toidx]*within) *amp; // Scale sample 1 to sample 2
			
			c += d;
			if (c > max)
				ticks = 0;
		}
		return value;
	}
    inline void to(double &tosample) { tosample += sample(); }
};

#endif // JUMP_SOUND_H