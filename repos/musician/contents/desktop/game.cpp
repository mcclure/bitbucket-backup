/*
 *  game.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/27/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "game.h"
#include "display_ent.h"
#include "test_ent.h"
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "display_ent.h"
#include "glCommonMatrix.h"
#include "util_pile.h"
#include "plaidext.h"
#include "util_thread.h"
#include "internalfile.h"
#include <vector>

#define SCRUNCH(x) ((x)*AudioFormat::INT24_CLIP)
#define WAVSCRUNCH(x) (cpfclamp((x),-1, 1)*SHRT_MAX)
#define STRETCH(x, high) ((high)*(cpfclamp(x,-1, 1)+1)/2)
#define DARKGRAY 0xFF444444
#define GRAYOFF  0x00000040
#define OPAQUE   0xFF000000
#define YELLOW 0xFF00FFFF 

#define HISTORY_SIZE (44100*10)

struct keyboard_state : public ent {
	uint32_t inputcode; bool use_scancode; int maxcode;
	bool *codes;
	
	keyboard_state(uint32_t _inputcode, bool _use_scancode=false, int _maxcode=-1) : ent(), inputcode(_inputcode), use_scancode(_use_scancode) {
		if (_maxcode < 0)
			maxcode = use_scancode ? 256 : SDLK_LAST;
		else
			maxcode = _maxcode;
		codes = new bool[maxcode];
		clear();
	}
	void clear() {
		for(int c = 0; c < maxcode; c++)
			codes[c] = false;
	}
	void forget(int code) {
		if (code < maxcode)
			codes[code] = false;
	}
	bool down(int code) {
		return code < maxcode && codes[code];
	}
	~keyboard_state() {
		delete[] codes;
	}
	void input(InputData *d) {
		if (d->inputcode == inputcode) {
			SDL_keysym key = d->key.keysym;
			int code = use_scancode ? key.scancode : key.sym;
			if (code < maxcode) {
				codes[code] = d->key.state == SDL_PRESSED; // PRESSED == no repeat
				ERR("CODE %d = %d\n", (int)code, (int)codes[code]); 
			}
		}
	}
};

// Not endian safe. Mono.
void wav_write(const char *filename, const float *values, int samples) {
	#define WAV_WRITE_CHUNK_SIZE 256
	uint16_t buffer[WAV_WRITE_CHUNK_SIZE];
	
	FILE *file = fopen(filename, "wb");
	{ // See https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
		uint32_t d4; uint16_t d2;
		const int samplerate = 44100;
		const int channels = 1;
		const int samplesize = sizeof(uint16_t);
#define W4( x ) d4 = (x); fwrite(&d4, sizeof(d4), 1, file);
#define W2( x ) d2 = (x); fwrite(&d2, sizeof(d2), 1, file);
#define WS( x ) fwrite((x), 1, 4, file);
		
		WS("RIFF");
		W4(36 + samples*samplesize); // Data + header
		WS("WAVE");
		WS("fmt ");
		W4(16); // Magic number -- WAVE section size
		W2(1); // Magic [indicates PCM]
		W2(channels); // # of channels
		W4(samplerate); // Sample rate
		W4(samplerate * channels * samplesize); // Bytes/sec
		W2(channels * samplesize); // Block alignment ?? wav is stupid
		W2(samplesize*8);
		WS("data");
		W4(samples*samplesize);
	}
	int progress = 0;
	while (progress < samples) {
		int used = 0;
		while (used < WAV_WRITE_CHUNK_SIZE && progress < samples)
			buffer[used++] = WAVSCRUNCH(values[progress++]);
		fwrite(buffer, used, sizeof(short), file);
	}
	fclose(file);
}

#include "buttons.h"

enum indicator_type {
	indicate_wave,
	indicate_plus,
	indicate_filter,
	indicate_rec,
	indicate_arrowl,
	indicate_arrowr,
	indicate_arrowup,
	indicate_arrowdown,
	indicate_skip,
	indicate_file,
	indicate_last
};

const char *indicator_names[indicate_last] = {
	"wave.png", "plus.png", "filter.png", "rec.png", "arrowl.png", "arrowr.png", "arrowup.png", "arrowd.png", "skip.png", "file.png"
};

struct BigSynth : public BaseSynth
{
	BigSynth(AudioFormat format, float frequency, float amp, int _windowSize, keyboard_state *_kbd) :
		BaseSynth(format, frequency, amp), s(0), sample(0), window_at(0), history_at(0), kbd(_kbd) {
		window.input->resize(_windowSize);
		window.output->resize(_windowSize);
		history.resize( HISTORY_SIZE );
		for(int c = 0; c < indicate_last; c++)
			presence[c] = -1000;
	}
	
	int s;
	int sample, result;
	
	int window_at;
	scalar_swap< vector<float> > window;
	
	int history_at;
	vector<float> history;
	
	keyboard_state *kbd;
	
	int presence[indicate_last]; // statePresence is "ticks value when last seen"
	
	void pull(AudioChunk &chunk, const State &a, const State &b) {
		int32_t *i = chunk.start(0), *e = chunk.end(0);

		while (i != e) {
			double x = 0;
			
			// HANDLE ALL KEYDOWNS
			// Synths
			if (kbd->down(button_squirm)) {
				x += sin( (2*M_PI*(440 + sin(sample/44100.0)) * double(sample))/44100.0 );
			}
			
			// Effects
			if (kbd->down(button_echo)) {
				#define ECHO_NUM (5)
				for(int c = 1; c < ECHO_NUM; c++) {
					float &echo = history[(history_at + HISTORY_SIZE - c*512)%HISTORY_SIZE];
					x += echo/ECHO_NUM;
				}
			}
			
			sample++;
			x = history_mix(x);
			eat_input(x);
			*i = SCRUNCH( x );

			i++;
		}

		if (kbd->down(button_squirm)) // ALL GENERATORS
			presence[indicate_wave] = ticks;
		
		if (kbd->down(button_echo)) // ALL FILTERS
			presence[indicate_filter] = ticks;

		#define SKIP_SIZE (HISTORY_SIZE/4)
		if (kbd->down(button_skip)) {
			sample += SKIP_SIZE;
			history_at += SKIP_SIZE;
			history_at %= HISTORY_SIZE;
			presence[indicate_skip] = ticks;
		}
		
		if (kbd->down(button_save)) {
			wav_write("output.wav", &history[0], HISTORY_SIZE);
			kbd->forget(button_save);
			presence[indicate_file] = ticks;
		}

		if (kbd->down(button_clear)) {
			for(int c = 0; c < HISTORY_SIZE; c++)
				history[c] = 0;
		}
		
		BaseSynth::pull(chunk, a, b);
	}
	
	float history_mix(float f) {
		float &history_point = history[history_at];
		history_point += f;
		history_at++;
		history_at %= HISTORY_SIZE;
		return history_point;
	}
	
	void eat_input(float f) {
		if (window.input->empty())
			return;
		(*window.input)[window_at] = f;
		window_at++;
		if (window_at >= window.input->size()) {
			try_locked lock(window.inuse);
			if (lock) {
				window.swap();
			}
			window_at = 0;
		}
	}
	void illustrate(texture_slice *s, bool is_history) {
		const vector<float> &source = (is_history ? history : *window.output);
		testfill(s, DARKGRAY);
		int lasty = -1;
		float xstretch = 1;

		if (!is_history) window.inuse.lock();

		#define SWEEPSIZE (40)
		if (is_history) { // "CLOCK"
			int xb = float(sample % source.size())/(source.size()-1) * s->width;
			xb += s->width;
			for (int xi = 0; xi < SWEEPSIZE; xi++) { // Magic number
				int x = (xb - xi)%s->width;
				float intensity = float(SWEEPSIZE-xi)/SWEEPSIZE; intensity *= intensity;
				uint32_t gray = GRAYOFF*intensity; gray %= 0xFF;
				uint32_t color = DARKGRAY;
				for(int c = 0; c < 3; c++) // This is total garbage.
					color += gray << (8*c);
				testcol(s, x, color);
			}
		}

		if (source.size() != s->width) {
			xstretch = float(source.size()/s->width);
		}
		
		for(int x = 0; x < source.size() && x < s->width; x++) {
			int sx = x * xstretch;
			int y = int(STRETCH(source[sx], s->height-1));
			
			if (lasty < 0 || lasty == y) {
				s->pixel[x][ y ] = YELLOW;
            } else { // Draw a clumsy line
                int dir = 1;
                int cap = lasty-y;
                if (cap < 0) {
                    cap = -cap;
                    dir = -1;
                }
                for(int y2 = 0; y2 < cap; y2++) {
                    s->pixel[x][ y+y2*dir ] = YELLOW;
                }
            }
			lasty = y;
		}
		
		if (!is_history) window.inuse.unlock();
		
		s->construct();
	}
};

// Put everything here
struct player : public ent {
	Ref<BigSynth> synth;
	fixed_splatter *scope, *history;
	display_gate *indicators[indicate_last];
	keyboard_state *kbd;
	player() : ent() {
		cpVect scopeScale = cpv(0.8/aspect, 0.3);
		cpVect scopeSize = cpvscale( cpv(surfacew, surfaceh), scopeScale);
		
		texture_slice *scope_texture = new texture_slice(); scope_texture->init(scopeSize.x, scopeSize.y);
		testfill(scope_texture, DARKGRAY);
		scope_texture->construct();
		scope = new fixed_splatter(new texture_source(scope_texture, true), true, NULL, scopeScale, cpv(0, -scopeScale.y*2));

		texture_slice *history_texture = new texture_slice(); history_texture->init(scopeSize.x, scopeSize.y);
		testfill(history_texture, DARKGRAY);
		history_texture->construct();
		history = new fixed_splatter(new texture_source(history_texture, true), true, NULL, scopeScale, cpv(0, scopeScale.y*2));
		
		kbd = new keyboard_state( INPUTCODE(G_INSTRUMENT, I_VACUUM) );
		
		synth = new BigSynth(audio->format(), 0, 1, scopeSize.x, kbd);

		int indicator_size_lowbound = scopeSize.y/3;
		int indicator_size_px = 32; // known size of PNGs
		while (indicator_size_px < indicator_size_lowbound)
			indicator_size_px += 32;
		float indicator_size = float(indicator_size_px)/surfaceh;
		float x = -(indicator_size * ( (indicate_last-2) - 1) * 2 + indicator_size)/2; // xbase
		for(int c = 0; c < indicate_last; c++) {
			char filename[FILENAMESIZE];			
			internalPath(filename, indicator_names[c]);
			texture_source *src = new texture_source(filename);
			fixed_splatter *splatter = new fixed_splatter(src, true, NULL, cpv(indicator_size,indicator_size), cpv(x, 0));
			if (c != indicate_arrowl && c != indicate_arrowup)
				x += indicator_size*2;
			indicators[c] = new display_gate();
			splatter->insert(indicators[c]);
		}
	}
	void inserting() {
	
		audio->play(synth);
		
		kbd->insert(this);
		scope->insert(this);
		history->insert(this);
		
		for(int c = 0; c < indicate_last; c++) {
			indicators[c]->insert(this);
		}
	}

	void display(drawing *d) {
		synth->illustrate(scope->src->outTexture, false);
		
		synth->illustrate(history->src->outTexture, true);
		
#define LEDTEST 0
		for(int c = 0; c < indicate_last; c++) {
#if LEDTEST
			indicators[c]->visible = (ticks % indicate_last) == c; // Display everything in turn
#else
			indicators[c]->visible = ticks-synth->presence[c] < 2; // Display anything "recently used"
#endif
		}
		
#if !LEDTEST
	int tsf = ticks-synth->presence[indicate_file];
	if (tsf < FPS/2) { // Pulse 3 times over half a second
		indicators[indicate_file]->visible = ( tsf / int(FPS / 12) ) % 2;
	}
#endif
		
		ent::display(d);
	}
};

void game_init() {
	int mode = 1;
	cheatfile_load(mode, "mode.txt");
	
	int mute = 0;
	cheatfile_load(mute, "mute.txt");
	if (mute) audio->volume(0);
	
#ifndef SELF_EDIT
	SDL_ShowCursor(0);
#endif

	switch (mode) {
		case 1:
			ditch_setup_controls();
			(new player())->insert();
			break;
		case -1: {
			(new inputdump_ent(InputKindEdge))->insert();
		} break;
	};
}