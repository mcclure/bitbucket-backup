/*
 *  project.c
 *  "c part"
 *
 *  Created by Andi McClure on 8/22/13.  AND ME
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "project.h"

#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Sound

#define FREQ 22050//44100

#if LOG_AUDIO
FILE *audiolog = NULL;
#endif

// GENERAL SOUND MANAGEMENT

struct sounding {
	void (*process)(struct sounding *, double *value);
	void *state;
	int dead; // 0: Live 1: ready to reap 2: reaped
};

struct sample {
	double *data;
	int len;
};
struct sampleset {
	struct sample **samples; // Array of sample pointers
	int len;
};

#define MAXSOUNDS 20
#define MAXFREESOUNDS (MAXSOUNDS+5)

struct sounding livesound[MAXSOUNDS];
int live_count = 0;

int freesound[MAXFREESOUNDS];
int freesound_pull_at = 0; // Write from main thread thread read from main thread
int freesound_push_at = 0; // Write from sound thread read from sound thread

struct sounding *sound_make(void (*callback)(struct sounding *, double *value), int statesize) {
	int idx = freesound[freesound_pull_at++];
	if (idx >= live_count)
		live_count = idx + 1;
	livesound[idx].process = callback;
	if (statesize)
		livesound[idx].state = malloc(statesize);
	return &livesound[idx];
}
EXPORT void sound_release(void *_s) {
	struct sounding *s = _s;
	s->dead = 1;
}
EXPORT void sound_start(void *_s) {
	struct sounding *s = _s;
	s->dead = 0;
}

// Individual sounds

// Squarewave
struct square {
	double volume;
	int phase;
	double decay;
	int period;
};
void process_square(struct sounding *_s, double *value) {
	struct square *sq = _s->state;
	if (sq->volume > 0) {
		*value += sq->phase/sq->period%2 ? sq->volume : -sq->volume;
		sq->phase++;
		sq->volume = sq->volume - sq->decay;
	}
}
EXPORT void *square_make() {
	return sound_make(process_square, sizeof(struct square));
}
EXPORT void square_set(void *_s, double volume, double pitch, double decay) {
	struct square *sq = ((struct sounding *)_s)->state;
	sq->period = FREQ / pitch;
	sq->decay = decay;
	sq->volume = volume;
}

// SAMPLE

struct playsample {
	double volume;
	int phase;
	struct sample play;
	int repeatp;
};
void process_playsample(struct sounding *_s, double *value) {
	struct playsample *ps = _s->state;
	if (!ps->play.data) return;
	if (ps->phase >= ps->play.len) {
		if (!ps->repeatp) return;
		ps->phase = 0;
	}
	*value += ps->play.data[ps->phase++] * ps->volume;
}
EXPORT void *playsample_make() {
	return sound_make(process_playsample, sizeof(struct playsample));
}
EXPORT void playsample_set(void *_s, struct sample *play, double volume, int repeatp) {
	struct playsample *sa = ((struct sounding *)_s)->state;
	sa->phase = 0;
	sa->volume = volume;
	sa->repeatp = repeatp;
	sa->play = *play;
}
struct sample muted = {0,0};
EXPORT void playsample_mute(void *_s) {
	struct playsample *sa = ((struct sounding *)_s)->state;
	sa->play = muted;
}

// SAMPLE-STUTTER

struct stutter {
	double volume;
	int phase;
	struct sample play;
	int phase2;
	int every;
	int jump;
};
void process_stutter(struct sounding *_s, double *value) {
	struct stutter *ps = _s->state;
	if (ps->phase2++ >= ps->every) {
		ps->phase2 = 0;
		ps->phase = ps->phase + ps->jump;
		ps->phase %= ps->play.len;
		if (ps->phase < 0)
			ps->phase += ps->play.len;
	}
	if (ps->phase >= ps->play.len) {
		ps->phase = 0;
	}
	*value += ps->play.data[ps->phase++] * ps->volume;
}
EXPORT void *stutter_make() {
	return sound_make(process_stutter, sizeof(struct stutter));
}
EXPORT void stutter_set(void *_s, struct sample *play, double volume, int every, int jump) {
	struct stutter *sa = ((struct sounding *)_s)->state;
	sa->phase = 0; sa->phase2 = 0;
	sa->volume = volume;
	sa->every = every;
	sa->jump = jump;
	sa->play = *play;
}

// SAMPLE-SET

struct playset {
	double volume;
	int atset;
	int phase;
	int mode;
	int moded1, moded2;
	double modef1, modef2;
	struct sampleset play;
};
void process_playset(struct sounding *_s, double *value) {
	struct playset *ps = _s->state;
	if (ps->phase >= ps->play.samples[ps->atset]->len) {
		ps->atset++;
		ps->atset %= ps->play.len;
		ps->phase = 0;
	}
	*value += ps->play.samples[ps->atset]->data[ps->phase++] * ps->volume;
}
EXPORT void *playset_make() {
	return sound_make(process_playset, sizeof(struct playset));
}
EXPORT void playset_set(void *_s, struct sampleset *play, double volume, int mode, int moded1, int moded2, double modef1, double modef2) {
	struct playset *sa = ((struct sounding *)_s)->state;
	sa->phase = 0;
	sa->atset = 0;
	sa->volume = volume;
	sa->mode = mode;
	sa->moded1 = moded1;
	sa->moded2 = moded2;
	sa->modef1 = modef1;
	sa->modef2 = modef2;
	sa->play = *play;
}

// BROUGH

struct worms {
	double thingummy[5];
	double compression;
	double last; int i;
};
#define PI2 6.283
//this sucks
void process_worms(struct sounding *_s, double *value)
{
	struct worms *w = _s->state;
	double a;
	
	if (w->i++%2) { // Enforce 22khz
		*value = w->last;
		return;
	}

	w->thingummy[0] += (PI2*0.1/FREQ);
	while (w->thingummy[0]>6.283) w->thingummy[0]-=6.283;
	w->thingummy[1] += (PI2*8.0/FREQ)*(1.0+0.75*sin(w->thingummy[0]));
	while (w->thingummy[1]>6.283) w->thingummy[1]-=6.283;
	
	w->thingummy[2] += (PI2*440.0/FREQ)*(1.0+0.5*sin(w->thingummy[1]));
	while (w->thingummy[2]>6.283) w->thingummy[2]-=PI2;
	w->thingummy[3] += (PI2*440.0/FREQ)*(1.0+0.3*sin(w->thingummy[1]));
	while (w->thingummy[3]>6.283f) w->thingummy[3]-=6.283f;
	w->thingummy[4] += (PI2*440.0/FREQ)*(1.0+0.1*sin(w->thingummy[1]));
	while (w->thingummy[4]>6.283f) w->thingummy[4]-=6.283f;
	
	*value += sin(w->thingummy[2])*0.60;
	*value += sin(w->thingummy[3])*0.30*(1.0+0.75*sin(w->thingummy[1]));
	*value += sin(w->thingummy[4])*0.30*(1.0+0.85*sin(w->thingummy[0]));
	
#ifdef _WINDOWS
	a = ((*value)<0)?(-*value):(*value);
#else
	a = fabs(*value);
#endif
	if (a > 0.3)
		w->compression = min(w->compression, 0.3/a);
	else
	{
		double target;
		target = 1.0 - (max(a,0.2)-0.2)*(0.7);
		w->compression += (target - w->compression)*0.15;
	}
	*value *= w->compression;
	
	w->last = *value;
}
EXPORT void *worms_make() {
	struct sounding *s = sound_make(process_worms, sizeof(struct worms));
	struct worms *w = s->state;
	int i;
	for (i=0; i<5; ++i)
		w->thingummy[i] = 0.0;
	w->compression = 0.0;
	w->i = 0;
	return s;
}

#define DIGITALCLIP 1

// Len is in bytes
void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);
	
	int c,d; for(c = 0; c < slen; c++) { // Write your audio code in here. My default just fires a simple list of "sounding" objects:
		double value = 0;
		
		for(d = 0; d < live_count; d++) {
			if (!livesound[d].dead) {
				livesound[d].process(&livesound[d], &value);
			}
		}
        
#if NO_AUDIO
		value = 0;
#endif

#if DIGITALCLIP
        if (value < -1) value = -1;
        if (value > 1) value = 1; // Maybe add like a nice gate function... I dunno
#endif
        
		samples[c] = value*SHRT_MAX;
	}
	
	// After-the-fact: Reap
	for(d = 0; d < live_count; d++) {
		if (livesound[d].dead == 1) {
			free(livesound[d].state);
			livesound[d].state = NULL;
			livesound[d].dead = 2;
			freesound[freesound_push_at] = d;
			freesound_push_at++;
		}
	}
	
#if LOG_AUDIO
	if (audiolog) { // For debugging audio
		fwrite(stream, sizeof(short), slen, audiolog);
	}
#endif
}

// From mac framework SDL_audio.h. Dubious.
/* Audio format flags (defaults to LSB byte order) */
#define AUDIO_U8        0x0008  /* Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /* Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /* Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /* Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /* As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /* As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB

EXPORT void sound_init() {
	int i;
	int failure;
	SDL_AudioSpec desired;
#if LOG_AUDIO
	audiolog = fopen("/tmp/OUTAUDIO", "w");
#endif

	// Set up sound queue
	for(i = 0; i < MAXSOUNDS; i++) {
		livesound[i].state = NULL;
		livesound[i].dead = 2;
		freesound[i] = i;
		freesound_push_at++;
	}

	desired.freq = 44100;
	desired.format = AUDIO_S16LSB;
	desired.channels = 1; // mono
	desired.samples = 512; // Value picked at near random
	desired.callback = audio_callback;
	desired.userdata = NULL;
	failure = SDL_OpenAudio(&desired, NULL);
	if (failure < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
	}
}

// SAMPLE HANDLING

// DON'T CALL THIS
EXPORT struct sample *load_sample_raw(const char *filename) {
	long length = 0; 
	short *result_raw = NULL;
	double *result = NULL;
	const double amp = 1;
	struct sample *sample_result = NULL;
	int c;
	
	FILE *file = fopen(filename, "r"); // REALLY SHOULD DO SOMETHING IF THIS RETURNS NULL
	if (!file < 1) {
		fprintf(stderr, "Couldn't load raw sound file '%s'\n", filename);
		return NULL;
	}
	fseek( file, 0, SEEK_END ); length = ftell( file );
	fseek( file, 0, SEEK_SET );
	length /= 2; // 16 bit pcm, 2 bytes per sample
	result_raw = (short *)malloc(length*sizeof(short));
	fread(result_raw, sizeof(short), length, file);
	fclose(file);

	result = malloc(sizeof(double)*length);
	for(c = 0; c < length; c++)
		result[c] = ((double)result_raw[c])/SHRT_MAX * amp;

    free(result_raw);
    
	sample_result = malloc(sizeof(struct sample));
	sample_result->data = result;
	sample_result->len = length;
    return sample_result;
}

EXPORT struct sample *load_sample(const char *filename) {
	long length = 0; 
	short *result_raw = NULL;
	int channels;
	double *result = NULL;
	const double amp = 1;
	struct sample *sample_result = NULL;
	int c;
	
	length = stb_vorbis_decode_filename(filename, &channels, &result_raw);
	if (length < 1) {
		fprintf(stderr, "Couldn't load sound file '%s'\n", filename);
		return NULL;
	}
	if (channels <= 1) {
		result = malloc(sizeof(double)*length);
		for(c=0; c < length; c++)
			result[c] = ((double)result_raw[c])/SHRT_MAX * amp;
	} else {
		length /= 2;
		result = malloc(sizeof(double)*length);
		for(c=0; c < length; c++)
			result[c] = ( ((double)result_raw[c*2]) + ((double)result_raw[c*2+1]) ) /SHRT_MAX * amp / 2;
	}
    free(result_raw);
    
	sample_result = malloc(sizeof(struct sample));
	sample_result->data = result;
	sample_result->len = length;
    return sample_result;
}

EXPORT struct sample *crop(struct sample *s, int start, int len) {
	struct sample *sample_result = malloc(sizeof(struct sample));
	sample_result->data = s->data+start;
	sample_result->len = len;
    return sample_result;
}
EXPORT struct sampleset *make_sampleset(struct sample *samples[], int len) {
	struct sampleset *sample_result = malloc(sizeof(struct sampleset));
	sample_result->samples = samples;
	sample_result->len = len;
    return sample_result;
}
EXPORT void destroy_sample(struct sample *s) {
	free(s->data);
	free(s);
}
