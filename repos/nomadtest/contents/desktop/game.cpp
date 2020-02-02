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
#include "internalfile.h"

#define TILESIZE 8

template <class T> 
struct grid : public hash_map<pair<int,int>, T> {
	bool has(int x, int y) {
		const pair<int,int> &co = pair<int,int>(x,y);
		return this->count(co);
	}
	T get(int x, int y, T def = T()) {
		const pair<int,int> &co = pair<int, int>(x,y);
		if (this->count(co))
			return (*this)[co];
		return def;
	}
	void set(int x, int y, T v) {
		const pair<int,int> &co = pair<int,int>(x,y);
		(*this)[co] = v;
	}
	void del(int x, int y) {
		const pair<int,int> &co = pair<int,int>(x,y);
		this->erase(co);
	}
};

texture_slice * load_sheet(const string& _name) {
	char filename[FILENAMESIZE];
	internalPath(filename, _name.c_str());
	texture_slice *leak = new texture_slice(); // When am I gonna start refcounting
	leak->load(filename);
	return leak;
}

static texture_slice * sprite_sheet = NULL;
static texture_slice * ground_sheet = NULL;
static texture_slice * sky_sheet = NULL;
static void image_load() {
	sprite_sheet = load_sheet("nomad_sprite.png");
	ground_sheet = load_sheet("nomad_ground.png");
	sky_sheet = load_sheet("nomad_sky.png");
}

#define LEVEL_MOVE_AT 10
#define AUTOMOVE_FREQ 3

struct monster {
	int kind;
	int dir;
	monster(int _kind = 0, int _dir=-1) : kind(_kind), dir(_dir) {}
};

// WOW LOOK AT ALL THIS TYPING :(
pair<int,int> poff(int dir) {
	pair<int,int> co(0,0);
	if (dir >= 0)
		(dir%2 ? co.first : co.second) = dir/2 ? -1 : 1;
	return co;
}

cpVect cpvoff(int dir, cpVect by) {
	pair<int,int> off = poff(dir);
	return cpvscale( cpv(off.first, off.second), by);
}

pair<int,int> pmove(pair<int,int> co, int dir) {
	pair<int,int> off = poff(dir);
	co.first += off.first; co.second += off.second;
	return co;
}

// Put everything here
struct level : public ent {
	int w, h;
	tilepile *sky, *ground;
	display_code skyCode, groundCode;
	
	grid<bool> groundMap;
	grid<monster> spriteMap;
		
	// CACHED
	cpVect pixelCache, sizeCache, offsetCache;
	
	bool blocked(pair<int,int> co) {
		if (!groundMap.get(co.first,co.second) || spriteMap.has(co.first,co.second))
			return true;
		return false;
	}
	
	level(int _w, int _h) : ent(), w(_w), h(_h) {
		pixelCache = cpv(2.0f/w/TILESIZE, -2.0f/h/TILESIZE);
		sizeCache = cpv(2.0f/w, -2.0f/h);
		offsetCache = cpv(-1,1);
	
		sky = new tilepile(sky_sheet, TILESIZE, TILESIZE, sizeCache, offsetCache);
		sky->immortal = true;
		sky->zat(1);
		skyCode = display_code_unique();
		for(int c = 0; c < w*h; c++)
			sky->tile(random()%4, c%w, c/w);
			
		ground = new tilepile(ground_sheet, TILESIZE, TILESIZE, sizeCache, offsetCache);
		ground->immortal = true;
		groundCode = display_code_unique();		

		bool player = false;
		for(int c = 0; c < w*h; c++) {
			int x = c%w, y = c/w;
			cpFloat out = cpvdistsq(cpv(x,y), cpv(w/2, h/2))/(w*h/4);
			ERR("x %d y %d out %f\n", x, y, (float)out);
			if (random()/float(RANDOM_MAX) > out) {
				groundMap.set(x,y,true);
				ground->tile(random()%4, x, y );
				if (random()%5 == 0) {
					int pick = random()%(player?3:4);
					if (pick == 3)
						player = true;
					spriteMap.set(x, y, pick);
				}
			}
		}
		roll();
	}
	void roll() {
		
		typedef pair< pair<int, int>, monster> record;
		vector< record > contents;
		for(grid<monster>::iterator i = spriteMap.begin(); i != spriteMap.end(); i++) {
			contents.push_back(*i);
		}
		spriteMap.clear();
		for(vector<record>::iterator i = contents.begin(); i != contents.end(); i++) {
			monster v = i->second;
			pair<int,int> co = i->first;
			v.dir = -1;
			if (state == 0) {
				int base = random()%4;
				for(int c = 0; c < 4; c++) {
					int ndir = (base+c)%4;
					pair<int,int> into = pmove(co, ndir);
					if (!blocked(into)) {
						v.dir = ndir;
						co = into;
						break;
					}
				}
			}
			spriteMap.set(co.first, co.second, v);
		}

		next((state+1)%AUTOMOVE_FREQ, LEVEL_MOVE_AT); // Reset state
	}
	void display(drawing *d) {
		d->mvp = glm::mat4();
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glAlphaFunc(GL_GREATER, 0);
		glEnable(GL_ALPHA_TEST);
				
		d->insert_pile(skyCode, sky);
		d->insert_pile(groundCode, ground);

		tilepile &sprite = d->get_tilepile(sprite_sheet, TILESIZE, TILESIZE, sizeCache, offsetCache);
		sprite.zat(-1);
		cpVect toward = cpvmult(pixelCache, TILESIZE*(frame/float(LEVEL_MOVE_AT)-1));
		for(grid<monster>::iterator i = spriteMap.begin(); i != spriteMap.end(); i++) {
			sprite.tile(i->second.kind, i->first.first, i->first.second, cpvoff( i->second.dir, toward ));
		}
	}
};

struct specialstate : public state_common {
	float px;
	specialstate(float _px) : px(_px) {}	
	void set(pile &q, drawing *d) {
		Special(0);
		state_common::set(q, d);
		glUniform1f(p->uniforms[s_px], px);
			
		GLERR("Uniforms");
	}
};

// Put everything here
struct runner : public ent {
	int tileGoal;
	bool metGoal;
	runner(int _tileGoal) : ent(), tileGoal(_tileGoal), metGoal(false) {}
	void inserting() {		
		int xtile, ytile;
		int safeScale = 0;
		for(int testScale = 1, testingSize = tileGoal*TILESIZE;;testScale++) {
			if (testScale * testingSize < surfaceh)
				safeScale = testScale;
			else
				break;
		}
		if (safeScale) {
			ytile = tileGoal;
		} else {
			ytile = (surfaceh - surfaceh % TILESIZE)/TILESIZE;
			safeScale = 1;
		}
		int tilePixels = safeScale*TILESIZE;
		{
			xtile = (surfacew - surfacew % tilePixels)/tilePixels;
		}
		
		filter *f = new fixed_filter( new level(xtile,ytile), new specialstate(1.0/(xtile*TILESIZE*2)), xtile*TILESIZE, ytile*TILESIZE,
			cpv(float(xtile*tilePixels)/surfaceh, float(ytile*tilePixels)/surfaceh) );
		f->insert(this);
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

	image_load();
	
	int tiles = 16;
	cheatfile_load(tiles, "tiles.txt");

	switch (mode) {
		case 1:
			(new runner(tiles))->insert();
			break;
		case 2: {
			( new level(tiles,tiles) )->insert();
			break;
		} case -1: {
			(new inputdump_ent(InputKindEdge))->insert();
		} break;
	};
}