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
#include <plaid/audio/effects.h>
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "display_ent.h"
#include "glCommonMatrix.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_precision.hpp>
#include <deque>

using namespace plaid;
extern Ref<Audio> audio;

#define S 10.0

// Assuming 60 FPS
#define CUBE 6
#define INPUT_EVERY 0
#define ROTATE_EVERY 5
#define NOTE_EVERY 15
#define ROTATE_STEPS 9
#define QUEUE_LIMIT 2
#define VOLUME (1.0/CUBE)
#define F_WORLD_VERSION 0x1
#define WORLD_DAT "world.obj"

enum { // RunMode
	ModeFree,
	ModeTurn,
	ModeForward
};

struct TinyNeed {
	int code;
	float strength;
	TinyNeed(int _code, float _strength) : code(_code), strength(_strength) {}
};

struct cell {
	bool wall[CUBE];
	void wallfill(bool v) { for(int c = 0; c < CUBE; c++) wall[c] = v; }
	cell() { wallfill(true); }
};

struct runner;

typedef glm::ivec3 coord;
typedef hash_map<coord, cell> cellmap;
namespace __gnu_cxx {                                                                                             
	template<> struct hash< coord > // Allow STL strings with hash_map
	{ size_t operator()( const coord& v ) const { return hash< unsigned int >()( (v.x<<0)^(v.y<<10)^(v.z<<20) ); } };          
}         
float justSign(float f) {
	return f > 0 ? 1 : -1;
}

int toFaceNum(const coord &d) {
	if (d.x == 1)  return 0;
	if (d.x == -1) return 3;
	if (d.y == 1)  return 1;
	if (d.y == -1) return 4;
	if (d.z == 1)  return 2;
			       return 5; // if (d.z == -1)
}

coord fromFaceNum(int f) {
	const static coord result[CUBE] = {coord(1,0,0), coord(0,1,0), coord(0,0,1),
									   coord(-1,0,0), coord(0,-1,0), coord(0,0,-1), };
	return result[f];
}

string faceName(int f) {
	const static char* names[CUBE] = {
		"red",    //0
		"blue",   //1
		"purple", //2
		"yellow", //3
		"green",  //4
		"cyan",   //5
	};
	return names[f];
}
 
coord vround(glm::vec4 v) {
	return coord(round(v.x/v.w), round(v.y/v.w), round(v.z/v.w));
}
int antiface(int f) { return (f+3)%CUBE; }

struct attract : public ent {
	int expired;
	attract() : ent(), expired(-1) {}
	void display(drawing *d) {
		jcImmediateColor4f(1.0,1.0,1.0,1.0);	// For debug
		drawText("Splinecraft", 0, 0.5, 0, true, true);
		drawText("Arrow keys / spacebar to move", 0, -0.40, 0, true, true);
		drawText("Spacebar to start", 0, -0.60, 0, true, true);
	}
	void input(InputData *data) {
		int code = INPUTCODE_ID(data->inputcode);
		if (code == I_LASER) {
			expired = ticks; // Defer moving on by one frame to prevent double input processing
		}
	}
	void tick();
};

struct sonar : public ent {
	runner *parent;
	int delays[CUBE];
	Ref<Amp> osc[CUBE];
	sonar(runner *_parent) : ent(), parent(_parent) {
		int freq = 25;

		int soundtype = Oscillator::KLAXON;
		cheatfile_load(soundtype, "music.txt");

		for(int c = 0; c < CUBE; c++) {
			osc[c] = new Amp(new Oscillator(audio->format(), freq, soundtype, 1), 0);
			freq *= 2;
		}
		ZERO(delays);
	}
	void insert() {
		for(int c = 0; c < CUBE; c++)
			audio->play(osc[c]);
		roll();
		ent::insert();
	}
	void roll();
//	void tick() { ent::tick(); }
};

// Put everything here
struct runner : public ent, public freeze {
	// State
	int substate;
	deque<TinyNeed> inputQueue;
	sonar *sound;
	cellmap world;
	
	// Place
	glm::mat4 face;
	glm::mat4 offset; // Wiggle
	coord at;
	
	// Caches -- Invalidated every return-to-free
	coord forwardCache;
	cell *cellCache;
	int faceNumberCache;
	// Invalidated on resetFaceNumberCache (zaps, moves)
	int faceDistanceCache[CUBE];
	
	int faceDistance(int toward) {
		int c = 0;
		coord tryAt = at;
		cell *tryCell = cellCache;
		coord i = fromFaceNum(toward);
		while(!tryCell->wall[toward]) {
			tryAt = tryAt + i;
			tryCell = &world[tryAt];
			c++;
		}
		return c; // No tree yet
	}
	void resetFaceDistanceCache() {
		for(int c = 0; c < 6; c++) {
			faceDistanceCache[c] = faceDistance(c);
		}
	}
	void move(coord to) {
		at = to;
		cellCache = &world[at]; // SAFE?
		resetFaceDistanceCache();
	}
	
	runner() : ent(), freeze(), face(1.0), offset(1.0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
		sound = new sonar(this);
		
		f_loadfile(WORLD_DAT);
		move(coord(0,0,0));
		
		next(ModeFree); // Will set substate, faceCache
		
		sound->insert();
	}
	void display(drawing *d) {
		jcMultMatrixf(glm::value_ptr(face));
		jcMultMatrixf(glm::value_ptr(offset));
		jcTranslatef(at.x*S*2, at.y*S*2, at.z*S*2);
	
		quadpile3 &q = d->get_quadpile3(D_P3, false, true);
		for(cellmap::iterator i = world.begin(); i != world.end(); i++) {
			const coord &co = i->first;
			const cell &ce = i->second;
			#define CF(s, p) (((s) - co.p*2)*S)
			cpVect corners[4] = {cpv(CF(-1, x),CF(-1, y)), cpv(CF(-1, x),CF(1, y)), cpv(CF(1, x),CF(1, y)), cpv(CF(1, x),CF(-1, y))};
			uint32_t colors[CUBE] = {0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00};
			int faces[CUBE] = {0, 4, 3, 1, 2, 5};
			
			for(int c = 0; c < 4; c++) {
				if (ce.wall[faces[c]]) q.xysheet(corners[c], corners[(c+1)%4], CF(-1, z), CF(1, z), colors[c]);
			}
			if (ce.wall[faces[4]]) q.xyplane(corners[0], corners[2], CF(-1, z), colors[4]);
			if (ce.wall[faces[5]]) q.xyplane(corners[0], corners[2], CF(1, z), colors[5]);
		}
	}
	
	void input(InputData *data) {
		int code = INPUTCODE_ID(data->inputcode);
		
		if (code == I_WILLQUIT) {
			f_savefile(WORLD_DAT); // Queue nothing, just save and get out.
		} else {
			float strength = fabs(data->strength) > 0.1 ? data->strength : 0; // todo put this in inputdata
			strength = justSign(strength) * (1.0/ROTATE_STEPS); // DID I DO SOMETHING WEIRD HERE
			
			if (inputQueue.size() >= QUEUE_LIMIT)
				inputQueue.pop_back();
			
			inputQueue.push_back(TinyNeed(code, strength));
		}
	}

	void next(int _state, int _rollover=-1) {
		switch(_state) {
			case ModeFree: {
				forwardCache = vround( glm::vec4(0,0,1,1) * face );
				faceNumberCache = toFaceNum(forwardCache);
				ERR("Facing %s is %d (%s)%s\n", glm::to_string(forwardCache).c_str(), faceNumberCache, faceName(faceNumberCache).c_str(), cellCache->wall[faceNumberCache]?" (BLOCKED)":"");
			
				_rollover = INPUT_EVERY;
				substate = 0;
			} break;
			case ModeTurn: {
				_rollover = ROTATE_EVERY;
			} break;
		}
		ent::next(_state, _rollover);
	}

	void roll() {
		TinyNeed *need = inputQueue.size() ? &inputQueue.front() : NULL;
		
		switch (state) {
			case ModeFree:
				if (inputQueue.size()) {
					bool defer = true;
					if (need->code == I_LASER) {
						if (cellCache->wall[faceNumberCache]) {
							cell *newCell = &world[at + forwardCache];
							    cellCache = &world[at]; // Just in case we just invalidated cellCache?! No spec for hash_map invalidation
							cellCache->wall[faceNumberCache] = false;
							newCell->wall[antiface(faceNumberCache)] = false;
							resetFaceDistanceCache();
							
							// BAIL
							inputQueue.pop_front();
							defer = false;
						}
					}
					if (defer)
						next(ModeTurn);
				}
				break;
			case ModeTurn:				
				switch(need->code) {
					case I_ROT_X:
						face = glm::rotate(glm::mat4(1.0), 90*need->strength, glm::vec3(0,1,0)) * face;
						break;
					case I_ROT_Y:
						face = glm::rotate(glm::mat4(1.0), 90*need->strength, glm::vec3(1,0,0)) * face;
						break;
					case I_LASER:
						if (substate+1 >= ROTATE_STEPS) { // Ew
							offset = glm::mat4(1.0);
							move(at + forwardCache);
						} else {
							offset = glm::translate(offset, glm::vec3(forwardCache)*float(2*S*need->strength));
						}
						break;
				}
						
				substate++;
				if (substate >= ROTATE_STEPS) {
					inputQueue.pop_front();
					next(ModeFree);
					roll(); // RECURSE!
				} else {
					next(ModeTurn);
				}
			break;
		}
	}
	
	void f_load(FILE *f) {
		if (F_WORLD_VERSION != f_read32(f)) return;
		
		uint32_t roomcount = f_read32(f);
		for(int c = 0; c < roomcount; c++) {
			uint32_t x = f_read32(f), y = f_read32(f), z = f_read32(f); 
			cell &ce = world[coord(x,y,z)];
			for(int d = 0; d < CUBE; d++) {
				ce.wall[d] = f_read8(f);
			}
		}
	}
	void f_save(FILE *f) {
		f_write32(f, F_WORLD_VERSION);
		
		f_write32(f, world.size());
		for(cellmap::iterator i = world.begin(); i != world.end(); i++) {
			const coord &co = i->first;
			const cell &ce = i->second;
			for(int c = 0; c < 3; c++)
				f_write32(f, co[c]);
			for(int c = 0; c < CUBE; c++)
				f_write8(f, ce.wall[c]);
		}
	}
};

void attract::tick() {
	if (expired > 0 && ticks > expired) { // Wow this is gross.
		die();
		(new runner())->insert();
	}
}

void sonar::roll() {
	for(int c = 0; c < CUBE; c++) {
		float volume;
		if (delays[c] > 0) {
			volume = 0;
			delays[c]--;
		} else {
			volume = VOLUME;
			delays[c] = parent->faceDistanceCache[c]+1;
		}
		osc[c]->volume(volume);
	}
	
	next(0, NOTE_EVERY); // Loop
}

void game_init() {
//	if (!gl2) { (new lockout())->insert(); return; }
	
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
			(new attract())->insert();
			break;
		case 2:
			ditch_setup_controls();
			(new runner())->insert();
			break;
		case 3:
			(new testdiamond())->insert();
			break;

		case -1: {
			(new inputdump_ent(InputKindEdge))->insert();
		} break;
	};
}