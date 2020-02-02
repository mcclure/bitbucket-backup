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
#include  <algorithm>

// Put everything here

#define KBDDONE "Press space bar when done"
#define JOYDONE "Press start when done"

int screen_height = 256, screen_width = 256;
int level_height = 512, level_width = 512;

struct bouncer : public expiring_ent {
	int next_level;
	bouncer() : expiring_ent(), next_level(-1) {}
	void die() {
		ent::die();
		if (next_level>=0)
			make_game()->insert();
	}
	void kick() { next_level = 0; expire(); }
	virtual ent *make_game();
};

#include "attract_incl.cpp"

struct slicescreen : public bouncer {
	texture_slice *screen;
	string_ent *message;
	int pixelscale;
	slicescreen(texture_slice *_screen) : bouncer(), screen(_screen), message(NULL), pixelscale(1) {}
	~slicescreen() { delete screen; }
	void inserting() {
		for(;screen_height*(pixelscale+1)<=surfaceh && screen_width*(pixelscale+1)<=surfacew; pixelscale++);
		double scrsize = double(surfaceh)/(screen->height*pixelscale)/1.375; // MAGIC NUMBER IS ABSOLUTELY NOT ACCEPTABLE
		
		(new fixed_splatter(new texture_source(screen, false), true, NULL, cpv(scrsize,scrsize) ))->insert(this);
	}
	void setMessage(const string &s) {
		if (!message) {
			message = new string_ent(s, 0, -1+textHeight()+descenderHeight(), true);
			message->insert(this);
		} else {
			message->constant_text = s;
		}
	}
};

static screenshotter screenshot(0, NULL, "scrunch");

struct confirmer : public slicescreen {
	confirmer(texture_slice *_screen) : slicescreen(_screen) {}
	
	void inserting() {
		slicescreen::inserting();
		
		setMessage(global_mapper->found_controllers ? "Save your image? Left face button for yes, bottom button for no." : "Save your image? Press Y for yes, N for no.");
	}
	
	void input(InputData *data) {
		if (data->inputcode == INPUTCODE(G_GAM, IGAM_CONFIRM)) {
			texture_slice scaled; 
			scaled.init(screen->width*pixelscale, screen->height*pixelscale);
			slice_scale(&scaled, screen, pixelscale);
			scaled.construct();
			
			screenshot.quickTake(&scaled);
			kick();
		} else if (data->inputcode == INPUTCODE(G_GAM, IGAM_CANCEL)) {
			kick();
		}
		slicescreen::input(data);
	}
	
	
};

// EVERYTHING IS BRESENHAM

typedef glm::ivec2 point;

struct circler {
	int i, r, x, y, radiusError, x0, y0;
	circler(int _x0, int _y0) : i(0), r(1), x0(_x0), y0(_y0) { reset(); }
	void reset() { // Reset everything assuming fixed x0, y0, r
		x = r;
		y = 0;
		radiusError = 1-x;
	}
	point next() {
		point result;
		switch (i%8) {
			case 0: result = point(x + x0, y + y0); break;
			case 1: result = point(y + x0, x + y0); break;
			case 2: result = point(-x + x0, y + y0); break;
			case 3: result = point(-y + x0, x + y0); break;
			case 4: result = point(-x + x0, -y + y0); break;
			case 5: result = point(-y + x0, -x + y0); break;
			case 6: result = point(x + x0, -y + y0); break;
			case 7:
				result = point(y + x0, -x + y0);
				y++;
				if (radiusError < 0) {
					radiusError += 2 * y + 1;
				} else {
					x--;
					radiusError += 2 * (y - x + 1);
				}
				if (!(x >= y)) { // DONE!
					r++;
					reset();
				}
				break;
		}
		i++;
		return result;
	}
	point center() { return point(x0, y0); }
};

struct liner {
	point center, target, at, distance, sign; int err;
	liner(const point &_center, const point &_target) : center(_center), target(_target) {
		signsplit(target.x, center.x, distance.x, sign.x);
		signsplit(target.y, center.y, distance.y, sign.y);
		reset_target();
	}
	
	void reset() { err = (distance.x > distance.y ? distance.x : -distance.y)/2; }
	void reset_center() { at = center; reset(); } // Will never be used?
	void reset_target() { at = target; reset(); }
	void reset_custom(point in) { at = in; reset(); }
	
	point next() {
		point result = at;
		
		int lasterr = err;
		
		if (lasterr > -distance.x) {
			err -= distance.y;
			at.x += sign.x;
		}
		if (lasterr < distance.y) {
			err += distance.x;
			at.y += sign.y;
		}
		
		return result;
	}
	
	static void signsplit(int a, int b, int &abs, int &sign) {
		abs = a-b;
		if (abs < 0) { abs = -abs; sign = -1; }
		else { sign = 1; }
	}
	
	bool valid() { return target != center; }
};

bool slice_write(slice *s, point p, uint32_t color) {
	if (s->contains(p.x, p.y)) {
		s->pixel(p.x, p.y) = color;
		return true;
	}
	return false;
}

template <class T>
T *simple_slice(int width, int height) {
	T *s = new T(); s->init(width, height); testfill(s, 0);
	return s;
}

#define ACOEF  0.2
#define ADECAY 0.1
#define AMAX 5.0
struct accelerator { // This is less useful now that I've moved almost all its functionality out.
	accelerator() : s(NULL), v(0) {}
	sticker *s;
	float v;
	void tick() {
		v += s->strength*ACOEF;
	}
};

double real_fmod(double a, double b) {
	double result = fmod(a,b);
	if (result<0) result += b;
	return result;
}

point wrap_within(const glm::vec2 &dst, slice *bounds) {
	point result;
	result.x = real_fmod(dst.x, bounds->width);
	result.y = real_fmod(dst.y, bounds->height);
	return result;
}

point wrap_within(const point &dst, slice *bounds) {
	point result;
	result.x = safe_imod(dst.x, bounds->width);
	result.y = safe_imod(dst.y, bounds->height);
	return result;
}

#define INTERNAL_SAND true
struct laser {
	laser(point _at = point(), int _err = 0, bool _escaped = false) : at(_at), err(_err), escaped(_escaped) {}
	point at;
	int err;
	bool escaped;
	void toLine(liner &line) const { line.at = at; line.err = err; }
	static void pull(vector<laser> &lasers, liner &line, slice *ball, int &caught) { // Did I really *NEED* to split this out?
		laser last; bool any = false, have = false, pushed = false, pending = false;
		point base = line.at;
		while(1) {
			point n = line.next();
			if (abs(n.x-base.x) > ball->width || abs(n.y-base.y) > ball->height)
				break;
			if (!ball->contains(n.x,n.y)) {
				1;
			} else if (ball->pixel(n.x,n.y)) {
				last = laser(n, line.err);
				have = true;
				any = true;
				pending = false;
			} else {
				pending = true;
				if (INTERNAL_SAND && have) {
					lasers.push_back(last);
					pushed = true;
					have = false;
				}
			}
		}
		if (!INTERNAL_SAND && have && pending) {
			lasers.push_back(last);
			pushed = true;
		}
		if (pushed && pending) {
			lasers.back().escaped = true;
		}
		if (any)
			caught++;
	}
};

int caughtmax_filter(int size, int skew) {
	int value = size+abs(skew);
	value = size + (value-size)/2;
	return value;
}

struct runner : public slicescreen {
	texture_slice *ball; // One day you will ascend
	slice *level;
	accelerator bx,by;
	glm::vec2 ball_at;
	bool firstTick;
	float bestAudio;
	int completed_at;
	bool mute;
	
	runner() : slicescreen(NULL), ball(NULL), level(NULL), firstTick(true), bestAudio(0), completed_at(-1), mute(muted->down) { audio_init(); }
	~runner() { delete level; } // screen owned by slicescreen, ball owned by confirmer
	
	void inserting() {
		screen = new texture_slice();
		screen->init(screen_height, screen_width);
		testcross(screen);
		screen->construct();
		
		ball = simple_slice<texture_slice>(screen_height, screen_width);
		ball_at = point((level_width-screen_width)/2, (level_height-screen_height)/2);
		level = simple_slice<slice>(level_height, level_width); // TODO: Bigger
		for(int i = 0; i < 4; i++) {
			for(int t = 0; t < level->height; t++) { // FIXME: NOT RECTANGLE SAFE!
				for(int r = 0; r < level->height/8; r++) {
					uint32_t color = packHsv(t/double(level->height)*360, 1, 1-r/double(level->height));
					switch(i) {
						case 0: level->pixel(t, r) = color; break;
						case 2: level->pixel(r, t) = color; break;
						case 1: level->pixel(t, level->height-r-1) = color; break;
						case 3: level->pixel(level->height-r-1, t) = color; break;
					}
				}		
			}
		}
		
		slicescreen::inserting();

		bx.s = new sticker(INPUTCODE(G_GAM, IGAM_MOVE_X)); bx.s->insert(this);
		by.s = new sticker(INPUTCODE(G_GAM, IGAM_MOVE_Y)); by.s->insert(this);

		audio_insert();
	}
	void tick() {	
		// DYNAMICS
		{
			glm::vec2 v = glm::vec2(bx.v, by.v);
			float l = glm::length(v);
			if (l > 0) {
				float l2 = ::max<float>(0, l - ADECAY);
				v = glm::normalize(v) * l2;
				bx.v = v.x; by.v = v.y;
			}
		}
		bx.tick(); by.tick();
		{
			glm::vec2 v = glm::vec2(bx.v, by.v);
			float l = glm::length(v);
			if (l > 0) {
				float l2 = ::min<float>(l, AMAX);
				v = glm::normalize(v) * l2;
				bx.v = v.x; by.v = v.y;
			}
		}
		
		ball_at += glm::vec2(bx.v, by.v);
		
		// GRAPHICS
		testfill(screen, 0xFFAA5511);

		point ball_offset = wrap_within(ball_at, level); // Center of circle in level space
		
		slice_blit(screen, ball);
		slice_blut(screen, level, -ball_offset.x, -ball_offset.y);
		
		// Draw here
		point offs = point(bx.v*10, by.v*10); // Expand, 'cuz.
		int caught = 0;
		int caughtmax = 1;
		if (offs != point()) {
			vector<laser> lasers;
			liner line(point(), offs);
			if (!firstTick) {
				if (fabs(offs.y) > fabs(offs.x)) {
					int y = offs.y > 0 ? 0 : ball->height-1;
					int skew;
					while (1) { point n = line.next(); if (abs(n.y) >= ball->height) break; skew = n.x; }
					caughtmax = caughtmax_filter(ball->width,abs(skew));
					for(int x = ::min<float>(-skew,0); x < ball->width+::max<float>(-skew,0); x++) {
						line.reset_custom( point(x, y) );
						laser::pull(lasers, line, ball, caught);
					}
				}
				if (fabs(offs.x) >= fabs(offs.y)) {
					int x = offs.x > 0 ? 0 : ball->width-1;
					int skew;
					while (1) { point n = line.next(); if (abs(n.x) >= ball->width) break; skew = n.y; }
					caughtmax = caughtmax_filter(ball->height,abs(skew));
					for(int y = ::min<float>(-skew,0); y < ball->height+::max<float>(-skew,0); y++) {
						line.reset_custom( point(x, y) );
						laser::pull(lasers, line, ball, caught);
					}
				}
			} else {
				lasers.push_back(laser(point(ball->height/2,ball->height/2), 0, true));
			}
			
			for(int c = 0; c < lasers.size(); c++) {
				const laser &l = lasers[c];
				l.toLine(line); line.next(); // FIXME: WHY IS NEXT HERE
				int maxdist = ::max( ::min(screen->width*2, screen->height*2), ::min(level->width*2, level->width*2) );
				uint32_t found = 0;
				int d; // Distance to goal
				for(d = 0; d < maxdist; d++) {
					uint32_t *pixel;
					if (l.escaped) {
						point n = wrap_within(line.next()+ball_offset, level);
						pixel = &level->pixel(n.x,n.y);
					} else {
						point n = line.next();
						if (!ball->contains(n.x,n.y))
							break;
						pixel = &ball->pixel(n.x,n.y);
					}
					if (*pixel) {
						found = *pixel;
						*pixel = 0;
						break;
					}
				}
				
				if (found) {
					l.toLine(line); line.next(); // FIXME: WHY IS NEXT HERE
					point ballpoint = line.next();
					if (ball->contains(ballpoint.x,ballpoint.y))
						ball->pixel(ballpoint.x,ballpoint.y) = found;
						
					l.toLine(line); line.next(); // FIXME: WHY IS NEXT HERE
					for(int e = 0; e < d; e++) {
						point n = line.next();
						if (screen->contains(n.x,n.y))
							screen->pixel(n.x,n.y) = found;
						else
							break;
					}
					
					firstTick = false;
				}
			}
			
			if (completed_at < 0) {
				if (caught >= caughtmax) {
					completed_at = ticks;
				}
			} else if (ticks-completed_at > 60*6) {
				if (message) {
					message->die();
					message = NULL;
				}
			} else if (!message && ticks-completed_at>60*2) {
				setMessage(global_mapper->found_controllers ? JOYDONE : KBDDONE);
			}
			
//			ERR("%d, %d\t", caught, caughtmax);
			bestAudio = ::max<float>( ::min<float>(float(caught)/caughtmax,1), bestAudio );
			audio_setting(bestAudio);
		}
		
		screen->construct();
		
		slicescreen::tick();
	}
	
	void input(InputData *data) {
		if (data->inputcode == INPUTCODE(G_GAM, IGAM_COMPLETE)) {
			kick();
		}
		slicescreen::input(data);
	}
	
	ent *make_game() {
		ball->construct();
		return new confirmer(ball);
	}
	
	Ref<Filter> filter;
	Ref<Pitch>  pitch;
	void audio_init() {
		if (mute) return;
		
		#define TONES 8
		Mixer *m = new Mixer(audio->format());
		Mixer *m2 = new Mixer(audio->format());
		int base=random()%110;
		int steps[3] = {9,4,7};
		int stepsize = steps[random()%3];
		ERR("Step size %d\n", stepsize);
		float majorstep = 2*pow(pow(2,1.0/12),double(stepsize));

		plaid::Uint32 synth = boris->down ? Oscillator::KLAXON : Oscillator::SQUARE;

		m ->play( new Amp( new Oscillator(audio->format(), base*1,           synth, 1.0/TONES ) ) );
		m ->play( new Amp( new Oscillator(audio->format(), base*1*majorstep, synth, 1.0/TONES ) ) );
		m2->play( new Amp( new Oscillator(audio->format(), base*2,           synth, 1.0/TONES ) ) );
		m2->play( new Amp( new Oscillator(audio->format(), base*2*majorstep, synth, 1.0/TONES ) ) );
		m ->play( new Amp( new Oscillator(audio->format(), base*4,           synth, 1.0/TONES ) ) );
		m ->play( new Amp( new Oscillator(audio->format(), base*4*majorstep, synth, 1.0/TONES ) ) );
		m2->play( new Amp( new Oscillator(audio->format(), base*8,           synth, 1.0/TONES ) ) );
		m2->play( new Amp( new Oscillator(audio->format(), base*8*majorstep, synth, 1.0/TONES ) ) );

		pitch = new Pitch( m2, 1 );
		m->play(pitch);
		
		filter = new Filter(
			new ModulateEffect(m, new Oscillator(audio->format(), 0.75, Oscillator::SINE, 1), 0, 0.25)
			, 300, 300);
			
		audio_setting(0);
	}
	void audio_insert() {
		if (mute) return;
		
		audio->play(filter);
	}
	void audio_setting(float percent) {
		if (mute) return;
		
		float mid = 300;
		percent = ::max<float>(0.001,percent);
		percent *= percent;
		filter->bandpass(mid-300*percent, mid+(44100-mid)*percent);
//		ERR("%lf\n", (double)percent);
		pitch->rate(percent);
	}
	void die() {
		if (!mute)
			audio->stop(filter);
		slicescreen::die();
	}
};

ent *bouncer::make_game() { return new runner(); }

void game_init() {
	int mode = 1;
	cheatfile_load(mode, "mode.txt");
	
	int mute = 0;
	cheatfile_load(mute, "mute.txt");
	if (mute) audio->volume(0);

	cheatfile_load(screen_height, "screen_height.txt");
	cheatfile_load(screen_width, "screen_width.txt");
	
#ifndef SELF_EDIT
	SDL_ShowCursor(0);
#endif

	switch (mode) {
		case 1:
			ditch_setup_controls();
			(new attract())->insert();
			break;
		case 2:
			ditch_setup_controls();
			(new runner())->insert();
			break;
		case -1: {
			(new input_vacuum_ent(InputKindEdge))->insert();
		} break;
	};
}