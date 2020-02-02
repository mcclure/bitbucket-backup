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
#include <plaid/audio.h>
#include <plaid/audio/synth.h>
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "rule.h"
#include "display_ent.h"

using namespace plaid;

extern Ref<Audio> audio;

#define NOTECAP 500
#define OSCS 8

#define SM 0.5

#define BRUSH_RADIUS 0.25

struct lifer;

struct player_space;
player_space *playerspace_singleton;

struct player_space : public chipmunk_ent {
	cpVect bounds;
	cpVect player[2];
	cpBody *player_body[2];
	player_space(cpVect _bounds) : chipmunk_ent(), bounds(_bounds) {
		playerspace_singleton = this;
		for(int c = 0; c < 2; c++) {
			player[c] = cpvzero;
			player_body[c] = NULL;
		}
		
		bounds = ynorm(bounds);
	}
	void space_init() {
		chipmunk_ent::space_init();

		for(int c = 0; c < 2; c++) {
			cpBody *body = cpBodyNew(5, INFINITY); // Maybe make massless?
			body->p = cpv( bounds.x*.1,bounds.y*.2 );
			body->v = cpv( 0, 0);
			player_body[c] = body;
			cpShape *shape = cpCircleShapeNew(body, 0.1, cpvzero);
			shape->e = 0.80;
			shape->u = 0.00;
			
			cpSpaceAddBody(space, body);
			cpSpaceAddShape(space, shape);
		}
	}
	
	void tick() {
		for(int c = 0; c < 2; c++) {
			cpVect goal = playerspace_singleton->player[c];
			player_body[c]->v = goal;
			
			const float xmax = bounds.x; // Wrap:
			if (player_body[c]->p.x+BRUSH_RADIUS < 0)    player_body[c]->p.x += (xmax+BRUSH_RADIUS*2);
			if (player_body[c]->p.x-BRUSH_RADIUS > xmax) player_body[c]->p.x -= (xmax+BRUSH_RADIUS*2);
			if (player_body[c]->p.y+BRUSH_RADIUS < 0)    player_body[c]->p.y += (1+BRUSH_RADIUS*2);
			if (player_body[c]->p.y-BRUSH_RADIUS > 1)    player_body[c]->p.y -= (1+BRUSH_RADIUS*2);
		}

		chipmunk_ent::tick();
	}
	
	void player_direct(int p, float x, float y) {
		if (p < 2)
			player[p] = cpvmult(cpv(x,y), 1);
	}
	
	void input(InputData *data) {
		int p = INPUTCODE_GROUP(data->inputcode); p--;
		int code = INPUTCODE_ID(data->inputcode);
		
		if (!(p == 0 || p == 1)) return;
		
		float strength = fabs(data->strength) > 0.1 ? data->strength : 0;
		
		switch (code) {
			case I_MOVE_X:
				player_direct(p, strength, player[p].y);
				break;
			case I_MOVE_Y:
				player_direct(p, player[p].x, -strength);
				break;
		}
	}
};

struct rule_splatter : public fixed_splatter {
	rule *rule_src;
	rule_splatter(rule *_rule_src, cpVect _size = cpv(1,1), cpVect _offset = cpvzero)
		: fixed_splatter(new texture_source(new texture_slice(), true), true, NULL, _size, _offset), rule_src(_rule_src) {
		src->outTexture->init(3, 3);
		blitrule();
	}
	void blitrule() {
		float low, span;
		rule_src->bounded(low, span); span -= low;
		span = span ? 1/span : 1;
		
		for(int x = 0; x < 3; x++)
			for(int y = 0; y < 3; y++)
				src->outTexture->pixel[x][y] = packGray((rule_src->v(x,y)- low)*span);
		src->outTexture->construct();
	}
	void display(drawing *d) {
		blitrule();
		fixed_splatter::display(d);
	}
};

struct specialstate : public stateoid {
	lifer *ctx;
	specialstate(lifer *_ctx) : ctx(_ctx) {}
	void set(const quadpile &);
};

struct wobbler : public switcher {
	float high, low, value;
	int last_flip;
	wobbler(uint32_t _inputcode, float _high, float _low, float _value = 0) : switcher(_inputcode), high(_high), low(_low), value(_value) {
		value = max(low, value);
	}
	void update(InputData *d) {
		last_flip = age();
	}
	void tick() {
		value = (value + (down ? low : high)) / 2;
	}
};

#define LIFERULES 3

struct lifer : public ent {
	uint32_t lastBeepTime, beepSpan;
	vector< Ref<Oscillator> > oscillators;

	alternator *target;
	player_space *physics;
	
	rule carule[LIFERULES];
	wobbler *radii[2];

	lifer() : ent(), lastBeepTime(0), beepSpan(10000), target(NULL), physics(NULL) {}
	
	void insert() {
		ent::insert();
		
		// AUDIO
		for(int c = 0; c < OSCS; c++) {
			Ref<Oscillator>oscillator = new Oscillator(audio->format(), 100+100*c, Oscillator::SINE, 1.0/OSCS);
			//Play the oscillator
			audio->play(oscillator);
			oscillators.push_back(oscillator);
		}
		
		#define INSET 100
		#define INSETINSET 20
		cpFloat py = 1.0/surfaceh;
		cpFloat ratio = float(surfaceh-INSET)/surfaceh;
		cpVect size = cpv(surfacew*ratio, surfaceh*ratio);
		cpVect offset = cpv(0, INSET);
		
		// PHYSICS
		physics = new player_space(size);
		physics->insert();
		
		// VIDEO
		texture_slice *drawInto = new texture_slice();
		drawInto->init(size.x*SM, size.y*SM);
		drawInto->construct();
		
		texture_slice *drawFrom = new texture_slice();
		drawFrom->init(size.x*SM, size.y*SM);
		teststatic(drawFrom);
		drawFrom->construct();
		
		rulecross(carule[0], .25, .05);
		carule[0].v(1,1) -= 1;
		
		ruleblur(carule[1]);
		carule[1].v(1,1) -= 1;
		
		target = new alternator(drawInto, true, drawFrom, true, new specialstate(this));
		target->insert(this, ENT_PRIO_FBTARGET);
				
		splatter *kid = new fixed_splatter(new texture_source(target->display_texture()), true, NULL, cpvmult(size, py),cpvmult(offset, py)); // Surfaceh to scale down
		kid->insert(this);
		
		cpFloat ilu_r = py*(INSET-INSETINSET);
		cpVect ilu_size = cpv(ilu_r, ilu_r);
		cpFloat ilu_y = (ilu_r + INSETINSET*py) - 1;
		cpFloat ilu_x = (surfacew/3)*py;
		
		for(int c = 0; c < 2; c++) {
			cpVect center = cpv(ilu_x*(c?1:-1), ilu_y);
			boxer *box = new boxer(ilu_size, center);
			box->insert();
			splatter *ilu = new rule_splatter(&carule[c], cpvsub(ilu_size, cpv(4*py,4*py)), center);
			ilu->insert(this);
		}
		
		radii[0] = new wobbler(INPUTCODE(G_PLAYER_L, I_S2), BRUSH_RADIUS, BRUSH_RADIUS*2); radii[0]->insert();
		radii[1] = new wobbler(INPUTCODE(G_PLAYER_R, I_S2), BRUSH_RADIUS, BRUSH_RADIUS*2); radii[1]->insert();
	}

	void tick() {
		ent::tick();
		
		uint32_t ticks = SDL_GetTicks();
//		int dist = ticks - lastBeepTime;
//		audio->volume( min<float>(pow(float(dist)/beepSpan,1),1.0) );
		for(int c = 0; c < oscillators.size(); c++) {
			float freq = oscillators[c]->frequency();
			freq += (2 + random()/float(RANDOM_MAX) - 0.5)*2*c;
			if (freq > 600) freq -= 500;
			oscillators[c]->frequency( freq );
		}
	}
		
	void input(InputData *data) {
		int p = INPUTCODE_GROUP(data->inputcode); p--;
		int code = INPUTCODE_ID(data->inputcode);
		
		if (!(p == 0 || p == 1)) return;
		
		rule &r = carule[p];
		
		switch (code) {
			case I_RULE_N:
				for(int c = 0; c < 3; c++) r.v(c, 0) += 0.1; r.offset_norm(0);
				break;
			case I_RULE_S:
				for(int c = 0; c < 3; c++) r.v(c, 2) += 0.1; r.offset_norm(0);
				break;
			case I_RULE_E:
				for(int c = 0; c < 3; c++) r.v(2, c) += 0.1; r.offset_norm(0);
				break;
			case I_RULE_W:
				for(int c = 0; c < 3; c++) r.v(0, c) += 0.1; r.offset_norm(0);
				break;
			case I_S1: {
//				r.v(1,1) += 0.2;
				#define AVG_BIG 0.75
				#define AVG_SMALL (1.0-AVG_BIG)
				float avg = r.power()/9;
				for(int x = 0; x < 3; x++) {
					for(int y = 0; y < 3; y++) {
						float & v = r.v(x, y);
						v = v*AVG_BIG + avg*AVG_SMALL;
					}
				}
				r.offset_norm(0);
			} break;
		}
	}
		
	void die() {
		ent::die();
	}
};

void specialstate::set(const quadpile &) {
	Special(0);
	texture_slice *s = ctx->target->display_texture();
	glUniform1f(p->uniforms[s_px], 1.0/s->twidth);
	glUniform1f(p->uniforms[s_py], 1.0/s->theight);
	glUniform1f(p->uniforms[s_width], s->coords[2]);
	glUniform1f(p->uniforms[s_height], s->coords[3]);
		
	cpVect tempv  = ctx->physics->player_body[0]->p;
	cpVect tempv2 = ctx->physics->player_body[1]->p;
	float playerv[4] = {tempv.x,tempv.y, tempv2.x,tempv2.y};
	glUniform2fv(p->uniforms[s_center], 2, playerv);

	float radius[2] = {ctx->radii[0]->value, ctx->radii[1]->value};
	glUniform1fv(p->uniforms[s_radius], 2, radius);
	
	GLfloat rules[9*LIFERULES];
	for(int c = 0; c < LIFERULES; c++)
		ctx->carule[c].unload(rules, c);
	
	glUniformMatrix3fv(p->uniforms[s_rule], LIFERULES, GL_FALSE, rules);
	
	GLERR("Uniforms");
}

void game_init() {
	if (!gl2) { (new lockout())->insert(); return; }
	
	int mode = 1;
	
	cheatfile_load(mode, "mode.txt");
	
#ifndef SELF_EDIT
	SDL_ShowCursor(0);
#endif

	switch (mode) {
		case 1:
			ditch_setup_controls();
			(new lifer())->insert();
			break;
		case 2:
			(new testdiamond())->insert();
			break;
		case 3: {
			player_space *s = new player_space(cpv(surfacew, surfaceh));
			s->insert();
			(new cp_debug_ent(s))->insert();
		} break;
		case 4: {
			(new inputdump_ent(InputKindEdge))->insert();
		} break;
	};
}