/*
 *  dclib.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 8/15/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "kludge.h"
#include <math.h>

#include "sound.h"
#include "dclib.h"

#include "chipmunk.h"
extern int ticks;
extern FILE *audiolog;

// Support

struct sounding { // I thought I was trying to use STL structures now...
    noise *from;
    sounding *next;
    int start;
    sounding(noise *_from, sounding *_next, int _start = 0) : from(_from), next(_next), start(_start) {}
    ~sounding() { delete from; }
};

circle mainCircle;

circle::circle() : soundroot(NULL), nowat(0), amp(1), playing(0), metro_at(METRO_AT_START), soundpaused(false), toggle_fire(false) {
	pthread_mutex_init (&live_mutex, NULL);
}


// Copy a pattern (but don't copy *everything*... and shallow-copy chips)
circle::circle(const circle &from) : soundroot(NULL), nowat(0), amp(from.amp), playing(0), metro_at(from.metro_at), soundpaused(false), toggle_fire(false) {
	pthread_mutex_init (&live_mutex, NULL);
	chips = from.chips;
}

const double halfstep = pow(2,1/12.0);

inline double chip_payload::f() { // Frequency (hz)
	return pow(halfstep, -this->p - (this->samp ? this->samp->pmod : 0));
}

// Actual audio callback

#define DIGITALCLIP 1

// This takes its circle as an argument, rather than being a method on circle, so it can be easily used as an SDL callback
void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);
	
	circle &p = userdata ? *((circle *)userdata) : mainCircle; // p for pattern
	
    int samplesPerLoop = p.samplesPerLoop();
	
    if (samplesPerLoop <= 0)
        return; // Whatever
    
#if 1
    if (p.toggle_fire) {
        p.soundroot = new sounding( new notone(p.chimeDuration()*4, 11, 0.1),
                                 p.soundroot, 0);
        p.toggle_fire = false;
        p.soundpaused = !p.soundpaused;
    }
    
    {
        double nowatstep = double(slen)/samplesPerLoop;
		
		//    int a = 0; // For err
        pthread_mutex_lock(&p.live_mutex);
        for(chip_iter i = p.chips.begin(); i != p.chips.end(); i++) {
			chip *ch = (chip *)i->second->data;
            double offset = ch->when;
			bool invisibly = false;
            
            double until = fmod(p.nowat, 1.0);
            if (until > offset) // Clumsy!
                offset += 1; // Switch offset to until for forever chimes
            until = offset - until;
            
			if (p.soundpaused) until = nowatstep;
			
			if (until >= nowatstep && ch->wildfire) {
				until = 0;
				ch->wildfire = false;
				invisibly = true;
			}
			
            if (until < nowatstep) {
				noise *tone;
				if (ch->ip.samp) {
					if (!ch->ip.p && !ch->ip.samp->pmod) { // Basic pitch
						tone = new sampletone(ch->ip.samp->samp, ch->ip.samp->len);
					} else {
						double f = ch->ip.f();
						// Find a fraction to approximate this number.
						// FIXME: This is obviously not the right way to do this.
						int n = 1,d = 1; // Numerator=Interpolate/Denominator=Decimate
						int nn =1,nd = 1; double nattempt = 0, ndist = INFINITY;
						while (n < 1024 && d < 1024) {
							double attempt = n/double(d);
							double dist = fabs(attempt-f);
							//							ERR("Seeking %lf:\t%d/%d\t=%lf\n",f,n,d,attempt);
							if (dist < ndist) {
								ndist = dist; nattempt = attempt; nn = n; nd = d; nattempt = attempt;
							}
							if (attempt == f) break; // Should never happen
							else if (attempt < f) // Too low; increase numerator
								n++;
							else if (attempt > f) // Too high; increase denominator
								d++;
						}
						//						ERR("Sought %lf:\t%d/%d\t=%lf\n",f,nn,nd,nattempt);
						tone = new resampletone(ch->ip.samp->samp, ch->ip.samp->len, n, d);
					}
				} else {
					double w = 100 * ch->ip.f()+1; // Pitch, if any
					tone = new sqtone(p.chimeDuration()*5, w, 0.1);
				}
				p.soundroot = new sounding( tone, p.soundroot, p.playing + until*slen);
				if (!invisibly)
					ch->fired = ticks;
            }
        }
        
		if (!p.soundpaused)
			p.nowat += nowatstep;
        pthread_mutex_unlock(&p.live_mutex);
    }
#endif
    
	for(int c = 0; c < slen; c++) {
		double value = 0;
		
        for(sounding **n = &p.soundroot; *n;) {
            sounding *now = *n;
            
            if (p.playing < (*n)->start) {
                n = &(*n)->next; // Slightly duplicative of code
                continue;
            }
            
            now->from->to(value);
            
            if (now->from->done()) {
                sounding *victim = *n;
                *n = (*n)->next;
                delete victim;
            } else {
                n = &(*n)->next;
            }
        }
		
        p.playing++;
		
#if NO_AUDIO
		value = 0;
#endif
		
#if DIGITALCLIP
		value *= p.amp;
        if (value < -1) value = -1;
        if (value > 1) value = 1; // Maybe add like a nice gate function... I dunno
#endif
        
		samples[c] = value*SHRT_MAX;
	}
	if (audiolog) { // For debugging audio
		fwrite(stream, sizeof(short), slen, audiolog);
	}
}
