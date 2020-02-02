// BUSINESS LOGIC

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2010 Andi McClure
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// File contains code from Chipmunk "MoonBuggy" demo; notice applies to that code only:
/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <dirent.h>

#include "chipmunk.h"
#include "color.h"
#include "program.h"
#include "tinyxml.h"
#include "sound.h"
#include "internalfile.h"
#include "bfiles.h"
#include "cpRect.h"

#define GRAVITY cpvzero
//#define GRAVITY cpv(0.0f, -9.8/2)

vector<spaceinfo> spaces;

bool wantClearUi = false;
WantNewScreen want = WNothing;
cpSpace *uiSpace = NULL;
vector<ContainerBase *> columns;

list<automaton *> automata;
list<event_automaton *> event_automata;
vector<automaton *> dying_auto;
vector<event_automaton *> dying_event_auto;

type_automaton *cli = NULL, *next_cli = NULL;

bool game_halt = false;
int game_halted_at = 0;
unsigned int show_dead = 0;
bool want_ending;

int mostrecent_button = -2;

// defaults is where the actual controls live. file_defaults is a cache of the contents of the
// controls file. this is so that if we have to disregard instructions from the controls file
// (eg because a joystick was not found) we can avoid overwriting that slot.
player_controls file_defaults[MAX_PLAYERS], defaults[MAX_PLAYERS];
string file_jnames[MAX_PLAYERS];

unsigned int playerColor[4] = {packColor(0,1,0,1), packColor(0,1,1,1), packColor(0.75,0.75,0.75,1), packColor(1,0.5,1,1)}; // Now this is just silly.
unsigned int playerShotw[2] = {20,40};

void automaton::tick() {
	frame++; // Maybe up to 0
    if (rollover >= 0 && frame > rollover) {
        state++;
		frame = 0;
    }
}

void automaton::die() {
    dying_auto.push_back(this);
}

bannerstream::~bannerstream() {
    displaying = str();
    displaying_life = life;
}

cpBB cpBBadd(cpBB src, cpVect plus) {
	return cpBBNew(src.l+plus.x,src.b+plus.y,src.r+plus.x,src.t+plus.y);
}

// /------ State used by the Snap ------\

extern hash_map<string, texture_atlas *> atlas_root;

// \------ begin actual code ------/

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	if (cli) // if there's a cli, it decides what to do with esc.
		cli->BackOut();
	else	 // if there's NO cli, then we're in a game and we should back up to a cli:
		Quit();
		//terminate_game();
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	// Default implementation does nothing.
}

void appendLevel(double gravity) {
	int s = spaces.size();
  /* We first create a new space */
  spaces.push_back(spaceinfo(s));
  
	spaces[s].space = cpSpaceNew();
	
	/* Next, you'll want to set the properties of the space such as the
     number of iterations to use in the constraint solver, the amount
     of gravity, or the amount of damping. In this case, we'll just
     set the gravity. */
	spaces[s].space->gravity = cpv(0, gravity);
	
	/* This step is optional. While you don't have to resize the spatial
	 hashes, doing so can greatly increase the speed of the collision
	 detection. The first number should be the expected average size of
	 the objects you are going to have, the second number is related to
	 the number of objects you are putting. In general, if you have more
	 objects, you want the number to be bigger, but only to a
	 point. Finding good numbers to use here is largely going to be guess
	 and check. */
	cpSpaceResizeStaticHash(spaces[s].space, 0.1, 2000);
	cpSpaceResizeActiveHash(spaces[s].space, 0.1, 200);
	
  /* This is the rigid body that we will be attaching our ground line
     segments to. We don't want it to move, so we give it an infinite
     mass and moment of inertia. We also aren't going to add it to our
     space. If we did, it would fall under the influence of gravity,
     and the ground would fall with it. */
  spaces[s].staticBody = cpBodyNew(INFINITY, INFINITY);
}

// This is called once when the program begins. Clear out and replace with your own code
void
program_init(void)
{
    cpInitChipmunk();
	uiSpace = cpSpaceNew();
	    
	// WTF
	defaults[0].controls[0].key = (SDLKey)'a';
	defaults[0].controls[1].key = (SDLKey)'w';
	defaults[0].controls[2].key = (SDLKey)'d';
	defaults[0].controls[3].key = (SDLKey)'s';
	defaults[0].controls[4].key = SDLK_LSHIFT;
	defaults[0].controls[5].key = (SDLKey)0;
	defaults[0].controls[6].key = (SDLKey)0;
	defaults[0].controls[7].key = (SDLKey)0;
	defaults[0].controls[8].key = (SDLKey)'q';
	defaults[1].controls[0].key = (SDLKey)'j';
	defaults[1].controls[1].key = (SDLKey)'i';
	defaults[1].controls[2].key = (SDLKey)'l';
	defaults[1].controls[3].key = (SDLKey)'k';
	defaults[1].controls[4].key = (SDLKey)'/';
	defaults[1].controls[5].key = (SDLKey)0;
	defaults[1].controls[6].key = (SDLKey)0;
	defaults[1].controls[7].key = (SDLKey)0;	
	defaults[1].controls[8].key = (SDLKey)'o';	
	
	defaults[0].auto_aim = true;
	defaults[1].auto_aim = true;
	
	file_defaults[0].jid = -2; // -2-- magic number for "don't load"
	file_defaults[1].jid = -2;
	
	load_controls(); // Will possibly overwrite defaults
		
    program_reinit();
}

void
program_reinit(void)
{
	extern bool gl2;
	if (1 || (gl2 && GLEE_EXT_framebuffer_object)) // UMAYBE?
		newtype(new logo_automaton());
	else
		newtype(new failure_automaton());
}

/* //UFIXME 
spaceinfo *spaceRightNow = NULL;
void objectPositionFunc(struct cpBody *body, cpFloat dt) {
	spaceinfo *s = spaceRightNow;
	live_info *i = (live_info *)body->data;
	
	int t = s->time() - i->life->occur;
	
	if (t < i->p.size()) {
		body->p = i->p[t];
	}
}

void objectVelocityFunc(struct cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt) {
	spaceinfo *s = spaceRightNow;
	live_info *i = (live_info *)body->data;
	
	int t = s->time() - i->life->occur;
	
	if (t < i->v.size()) { // Spring to life
		body->v = i->v[t];
	}
}
*/

void blockVelocityFunc(struct cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt) {
	spaceinfo *s = &spaces[0]; // MESSY
	block_info *i = (block_info *)body->data;
	
	cpVect desv;
	
	if (ticks - i->updated > 0) {
		desv = cpvzero;
	} else {
		desv = cpvmult( cpvsub(i->wantp,body->p), 1/DT );
	}
	
	body->v = desv;
}

cpVect force_spawn = cpvzero;

player_info * make_player(spaceinfo *s, level_idiom *i, cpVect p, cpFloat rad, unsigned int color) {
	player_info *info = new player_info();
	info->color = color;
	info->spaces.resize(spaces.size());
	// p and v get filled in later?
	
	if (force_spawn.x || force_spawn.y) p = force_spawn; // If reset by watch_file
	
	cpBody *newBody = cpBodyNew(5, INFINITY);
	newBody->data = info;
	newBody->p = p;
	newBody->v = cpvzero;
//		newBody->velocity_func = objectVelocityFunc;
//		newBody->position_func = objectPositionFunc;
	
#if 0
	rad = PLAYER_RAD;
#endif
	
	cpVect player_verts[] = SQUARE(rad);
	
	cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
	newShape->e = i->p_e;
	newShape->u = i->p_u;
	newShape->collision_type = info->collision_type;
	newShape->layers = info->collision_interest;
	
	info->spaces[s->sid].body = newBody;
	info->spaces[s->sid].shapes.push_back(newShape);
	
	cpSpaceAddBody(s->space, newBody);
	cpSpaceAddShape(s->space, newShape);
	s->alive.push_back(info);

	return info;
}

#define DRAG_MASS 10

block_info * make_block(spaceinfo *s, level_idiom *i, cpRect cover, unsigned int lock = 0, unsigned int color = 0xFFFFFFFF) {
	block_info *info = new block_info();
	info->color = color;
	info->spaces.resize(spaces.size());
	info->lock = lock;
	
	cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
	newBody->data = info;
	newBody->p = cover.center;
	newBody->v = cpvzero;
	newBody->velocity_func = blockVelocityFunc;
	//		newBody->position_func = objectPositionFunc;
	
	cpVect player_verts[] = RECT(cover.rad.x,cover.rad.y);
	
	cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
	newShape->e = i->p_e;
	newShape->u = i->p_u;
	newShape->collision_type = info->collision_type;
	newShape->layers = info->collision_interest;
	
	info->spaces[s->sid].body = newBody;
	info->spaces[s->sid].shapes.push_back(newShape);
	s->alive.push_back(info);
	
	cpSpaceAddBody(s->space, newBody);
	cpSpaceAddShape(s->space, newShape);
	
	return info;
}

// Unused as of yet
live_info * make_slider(spaceinfo *s, level_idiom *i, cpRect cover, unsigned int color = 0xFFAAAAAA) {
	live_info *info = new live_info(C_SLIDER, C_SOLIDS);
	info->color = color;
	info->spaces.resize(spaces.size());
	
	cpBody *newBody = cpBodyNew(DRAG_MASS, INFINITY);
	newBody->data = info;
	newBody->p = cover.center;
	newBody->v = cpvzero;
//	newBody->velocity_func = blockVelocityFunc;
	//		newBody->position_func = objectPositionFunc;
	
	cpVect player_verts[] = RECT(cover.rad.x,cover.rad.y);
	
	cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
	newShape->e = i->p_e;
	newShape->u = i->p_u;
	newShape->collision_type = info->collision_type;
	newShape->layers = info->collision_interest;
	
	info->spaces[s->sid].body = newBody;
	info->spaces[s->sid].shapes.push_back(newShape);
	s->alive.push_back(info);
	
	cpSpaceAddBody(s->space, newBody);
	cpSpaceAddShape(s->space, newShape);
	
	return info;
}

// Dividers
void make_sep(spaceinfo *s, level_idiom *i, cpRect cover, unsigned int color = 0xFFFFFFFF) {
	switch_info *info = new switch_info(C_SEP, C_SEP, true);
	info->color = color;
	info->spaces.resize(spaces.size());	
	
	cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
	newBody->p = cover.center;
	newBody->v = cpvzero;
	newBody->velocity_func = blockVelocityFunc;
	newBody->data = info;
	//		newBody->position_func = objectPositionFunc;
	
	cpVect player_verts[] = RECT(cover.rad.x,cover.rad.y);
		
	cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
	newShape->e = i->p_e;
	newShape->u = i->p_u;
	newShape->collision_type = info->collision_type;
	newShape->layers = info->collision_interest;

	info->spaces[s->sid].body = newBody;
	info->spaces[s->sid].shapes.push_back(newShape);	
	
	cpSpaceAddStaticShape(s->space, newShape);
}

void make_button(spaceinfo *s, level_idiom *i, cpRect cover, unsigned int color = 0xFFFFFFFF) {
	switch_info *info = new switch_info(C_BUTTON, C_PLAYER|C_SLIDER, false);
	info->color = color;
	info->spaces.resize(spaces.size());	
	
	cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
	newBody->p = cover.center;
	newBody->v = cpvzero;
	newBody->data = info;
	
	cpVect player_verts[] = RECT(cover.rad.x,cover.rad.y);
	
	cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
	newShape->e = i->p_e;
	newShape->u = i->p_u;
	newShape->collision_type = info->collision_type;
	newShape->layers = info->collision_interest;
	
	info->spaces[s->sid].body = newBody;
	info->spaces[s->sid].shapes.push_back(newShape);	
	s->alive.push_back(info);

	cpSpaceAddStaticShape(s->space, newShape);
}

void make_exit(spaceinfo *s, level_idiom *i, cpRect cover, unsigned int color = 0xFFFFFFFF) {
	cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
	newBody->p = cover.center;
	newBody->v = cpvzero;
	
	cpVect player_verts[] = RECT(cover.rad.x,cover.rad.y);
	
	cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
	newShape->e = i->p_e;
	newShape->u = i->p_u;
	newShape->collision_type = C_EXIT;
	newShape->layers = C_EXIT|C_PLAYER;
		
	cpSpaceAddStaticShape(s->space, newShape);
}

int player_count = 1; // Note: player_count never changes, and I depend on this.

/*
level_idiom *level_idiom::clone() {
	level_idiom *i = new level_idiom();
	*i = *this;
	return i;
}
 */

extern cpRect arena;

level_idiom *load_svg(string filename, spaceinfo *s) {
	TiXmlDocument xml(filename);
	if (!xml.LoadFile()) return NULL;
	level_idiom *i = new level_idiom();
	
	TiXmlElement *rootxml = (TiXmlElement *)xml.IterateChildren("svg",NULL);
    if (!rootxml || rootxml->Type() != TiXmlNode::ELEMENT) return NULL;
	
	double w = 0,h = 0;
	rootxml->QueryDoubleAttribute("width", &w);
	rootxml->QueryDoubleAttribute("height", &h);
	ERR("LIVE%lf,%lf\n", w, h);
	
	// Everything is on 2x scale. Why? IDK, everything broke until I did this.
	// I only have 48 hours, I guess this time I'll have to let it slide...
	i->camera_limit = cpbounds(cpvzero,cpv(2 * w/h, 2));
	i->camera_visible = cpr(cpvzero, cpv(arena.rad.x/arena.rad.y,1));
	i->camera_nudge = i->camera_visible.inset(0.4);	

	// Only allow motion in the X direction.
	i->have_camera_freedom = 1;
	i->have_camera_limit = 1;
	
	i->std_zoom = 1;
	
    TiXmlElement *gxml = NULL;
    while(1) { // --- LAYER
        gxml = (TiXmlElement *)rootxml->IterateChildren("g", gxml);
        if (!gxml) break;
		if (gxml->Type() != TiXmlNode::ELEMENT) continue;
		ERR("G\n");
		TiXmlElement *obj = NULL;
		while(1) { // --- RECTANGLE
			obj = (TiXmlElement *)gxml->IterateChildren("rect", obj);
			if (!obj) break;
			if (obj->Type() != TiXmlNode::ELEMENT) continue;
			double bx, by, bw, bh;
			obj->QueryDoubleAttribute("x", &bx);
			obj->QueryDoubleAttribute("y", &by);
			obj->QueryDoubleAttribute("width", &bw);
			obj->QueryDoubleAttribute("height", &bh);
			ERR("RECT %lf %lf %lf %lf\n", bx, by, bw, bh);
			cpRect r = cpbounds(cpv(bx,h-by),cpv(bx+bw,h-(by+bh))); // Ugly y-flip
			r = r.scale(2/h);
			
			string kind = S(obj->Attribute("kind"));
			
			if (kind == "player") {
				s->player = make_player(s, i, r.center, r.rad.x, playerColor[0]);
			} else if (kind == "slider") {
				make_slider(s, i, r);
			} else if (kind == "sep") {
				make_sep(s, i, r);
			} else if (kind == "lock") {
				make_block(s, i, r, 1);
			} else if (kind == "button") {
				make_button(s, i, r);
			} else if (kind == "exit") {
				make_exit(s, i, r);
			} else { // Plain block
				make_block(s, i, r);
			}
		}
	}
	
	return i;
}

bool repeat = false;

// Start a new one
void game_start() {
	game_halt = false; game_halted_at = 0;
	glClearColor(1,1,1,1); // Bad habit
	SDL_ShowCursor(true);  // BAD HABIT
	map_init();
}

void game_stop() {
	force_spawn = spaces[0].player->spaces[0].body->p;
	
	for(int c = 0; c < player_handler::all_players.size(); c++)
		player_handler::all_players[c]->die();
	player_handler::all_players.clear();	
	
	killAllSound = true;
}

void game_reset() {
	game_stop();
	
	map_init();
}

void map_init() {
	spaces.clear(); // TODO free memory

	repeat = true;
	appendLevel(0); // Initial memory
	
	if (li) delete li;
	li = load_svg("level0.svg", &spaces[0]);
	if (li) {
		watch_file("level0.svg");
	} else {
		char filename[FILENAMESIZE]; liveInternalPath(filename, "level0.svg");
		li = load_svg(filename, &spaces[0]);
	}

    for(int s = 0; s < spaces.size(); s++)
		spaces[s].setup_collisions(); // Oh look if you resize spaces after doing this everything crashes later.

	cpVect offset_p = cpv(0,  0.5);
		
	cpVect ul; // Used for padded room
	ul = cpv(-1/aspect, -1);

	for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
		spaceinfo *s = &(*_s);
		cpRect room = li->camera_limit.inset_fixed(-li->outside_size/2);

		for(int c = 0; c < 4; c++) {
			cpRect line = cpbounds(room.vert(c), room.vert((c+1)%4)).inset_fixed(-li->outside_size/2);
			cpVect verts[4]; line.fill(verts);

			cpShape *newShape = cpPolyShapeNew(s->staticBody, 4, verts, cpvzero);
			cpSpaceAddStaticShape(s->space, newShape);
			newShape->e = li->w_e; newShape->u = li->w_u; newShape->collision_type = C_FIXED; newShape->layers = C_FIXED;
			newShape->group = GROUP_PREDESTINED;
		}
	}
	
	assert(spaces[0].player);
	
	{
		player_handler *k = new player_handler(spaces[0].player, 0);
		k->load(defaults[0]);
		k->camera_off.y += 1;
		k->camera_zoom = li->std_zoom;
		k->push();
				
		spaces[0].visible = 3; // UMAYBE?
	}
		
	di->process = 0; // Right now all process does is inset the per-player screens
}

void event_automaton::die() {
	if (halted) return;
	halted = true;
    dying_event_auto.push_back(this);
}

void player_info::be_alive_pre(spaceinfo *s) {
	cpBody *body = spaces[s->sid].body;
	cpVect desv = lastv;
	cpVect newv = cpvmult( cpvsub(desv,body->v), 1/DT ); // Very alarmed by this line (but I've forgotten why?).

	if (li->gravity) { // So that we don't cancel out gravity by "returning to zero"
		if (!desv.x && !desv.y)
			newv = cpvzero;
	}
	
	newv = cpvmult(newv, li->player_inertia);

	cpBodyResetForces(body);
	//		ERR("v %lf,%lf\n", newv.x,newv.y);
	cpBodyApplyForce(body, cpvmult(newv, body->m), cpvzero);	 
//	if (lastv.x || lastv.y)	ERR("c %lf, %lf\n", body->p.x, body->p.y);
}

void block_info::be_alive_pre(spaceinfo *s) {
	if (dragging) {
		cpVect lastClick = screenToGL(wantp_s.x, wantp_s.y, 0);
		wantp = cpvsub(lastClick, draggingInto);
		updated = ticks;
	}
}

void switch_info::be_alive_post(spaceinfo *s) {
	if (updated > mostrecent_button)
		mostrecent_button = updated;
}

bool switch_info::button_down() {
	return ticks-mostrecent_button < 2;
}

// The default display() calls this once per framedraw.
void
program_update(void)
{
    ticks++;
    
    // Automata management
    for(auto_iter i = automata.begin(); i != automata.end(); i++) {
        (*i)->tick();
    }        
    if (!dying_auto.empty()) {
        for(int c = 0; c < dying_auto.size(); c++) {
            automaton *a = dying_auto[c];
            automata.erase(a->anchor);
            delete a;
        }
        dying_auto.clear();
    }
	if (!dying_event_auto.empty()) {
        for(int c = 0; c < dying_event_auto.size(); c++) {
            event_automaton *a = dying_event_auto[c];
			automata.erase(a->anchor);
            event_automata.erase(a->event_anchor);
            delete a;
        }
        dying_event_auto.clear();
    }    
	if (next_cli) {
		newtype(next_cli);
		next_cli = NULL;
	}

	// FIXME: Should I bail out here if not in "game mode"?
	
	if (game_halt) return; // Yeah sure
	
	if (player_handler::all_players.size()) { // Assume this means game has started
		goOrtho(); // UGLY!! but needed for alive_pre
		player_handler::all_players[0]->apply_camera(true);
	}
	
	// Mechanics
	for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
		spaceinfo *s = &(*_s);
		
		for(vector<live_info *>::iterator a = s->alive.begin(); a != s->alive.end(); a++)
			(*a)->be_alive_pre(s);
		
#define SUBS 3
		for(int c = 0; c < SUBS; c++) {
			cpSpaceStep(s->space, DT/SUBS);
		}
		
		for(vector<live_info *>::iterator a = s->alive.begin(); a != s->alive.end(); a++)
			(*a)->be_alive_post(s);
	}
	
	// Calculate camera positions
	for(int _p = 0; _p < player_handler::all_players.size(); _p++) {
		player_handler *p = player_handler::all_players[_p];
		cpBody *body = p->player->spaces[p->sin].body;
		
		cpFloat t; // temp
		cpVect need = cpvzero;
		// First pass: move camera based on leaving "nudge" rectangle
		cpRect scr;
		
		if (li->have_camera_freedom) {
			scr = li->camera_nudge.inset(1/p->camera_zoom).translate(p->camera_off);
			if (li->have_camera_freedom&1) {
				t = body->p.x-scr.r(); if (t>0) need.x += t;
				t = scr.l()-body->p.x; if (t>0) need.x -= t;
			}
			if (li->have_camera_freedom&2) {
				t = body->p.y-scr.u(); if (t>0) need.y += t;
				t = scr.d()-body->p.y; if (t>0) need.y -= t;
			}
			
			p->camera_off = cpvadd(p->camera_off, need);
		}
		
		// Second pass: Confine camera based on limits rectangle
		if (li->have_camera_limit) {
			scr = li->camera_visible.inset(1/p->camera_zoom).translate(p->camera_off);
			need = cpvzero;
			
			if (li->have_camera_limit&1) {
				t = scr.r()-li->camera_limit.r(); if (t>0) need.x -= t;
				t = li->camera_limit.l()-scr.l(); if (t>0) need.x += t;
			}
			if (li->have_camera_limit&2) {
				t = scr.u()-li->camera_limit.u(); if (t>0) need.y -= t;
				t = li->camera_limit.d()-scr.d(); if (t>0) need.y += t;
			}
			
			p->camera_off = cpvadd(p->camera_off, need);
		}
		
		p->rtrigger_x = 0; p->rtrigger_y = 0;
		if (li->repeat) {
			scr = li->camera_visible.inset(1/p->camera_zoom).translate(p->camera_off);
			if (li->repeat&1) {
				t = scr.r()-li->camera_limit.r(); if (t>0) p->rtrigger_x = 1;
				t = li->camera_limit.l()-scr.l(); if (t>0) p->rtrigger_x = -1;
			}
			if (li->repeat&2) {
				t = scr.u()-li->camera_limit.u(); if (t>0) p->rtrigger_y = 1;
				t = li->camera_limit.d()-scr.d(); if (t>0) p->rtrigger_y = -1;
			}
		}
		if (p->rtrigger_x || p->rtrigger_y) {
//			ERR("%d. TRIGGER! %d, %d\n", ticks, p->rtrigger_x, p->rtrigger_y);
		}
	}
	
	check_updates(); // See internalFile.h
	
	if (want_ending) {
		game_stop();
		glClearColor(0,0,0,1);
		nexttype(new dismiss_automaton());
	}
}

// Mouse handling

cpVect lastClick = cpvzero; // FIXME // ... why did I leave these "FIXME" comments?
cpShape *draggingMe = NULL; // FIXME FIXME
cpVect draggingMeInto = cpvzero;

void mouseDownOnShape(cpShape *shape, void *data) {
    uintptr_t mask = (uintptr_t)data;
    if (shape->collision_type & mask) {
        cpVect clickedAt = cpvsub(lastClick, shape->body->p);
        if (draggingMe && cpvlengthsq(clickedAt) > cpvlengthsq(draggingMeInto)) // Only register closest click
            return;
        clickedAt = cpvunrotate( clickedAt, shape->body->rot );
        
        draggingMe = shape;
        draggingMeInto = cpvunrotate( cpvsub(lastClick, draggingMe->body->p), draggingMe->body->rot );
    }
}

void program_metamouse(dragtouch &d, touch_type t, cpVect at) {
	// I suggest keeping this little 4-line sigil at the top so that the interface library will work:
//	if (t == touch_down && interface_attempt_click(at, d.button)) // Let the interface library look at the event first
//		d.special = dragtouch::interface_special;
	if (d.special == dragtouch::interface_special) // Once a touch has been marked as part of the interface library, ignore it
		return;
	
	if (!spaces.size()) return;
	
    if (t == touch_up || t == touch_cancel) {
        if (d.dragging) {
            cpShape *wasDragging = d.dragging;
			cpBody *body = d.dragging->body; // CAN ASSUME IT'S A BLOCK?
			block_info *b = (block_info *)body->data;
			b->dragging = false;
			if (b->siren) {
				b->siren->period = b->siren->t2*2;
			}
			
			cpBodySetMass(body, INFINITY);
			
            wasDragging->body->v = cpvzero;
            
            d.dragging = NULL; // These two lines unnecessary on ipad, necessary on PC
            return; // Do for all cancels? (Irrelevant on PC)
        }
        d.special = dragtouch::not_special; // Mostly for the "mouse" dragtouch, which "survives"
	}
	goOrtho();
	player_handler::all_players[0]->apply_camera(true);
	d.lastClick = screenToGL(at.x, at.y, 0);
//	ERR("q %lf, %lf -> %lf, %lf\n", at.x,at.y, d.lastClick.x, d.lastClick.y);
	if (t == touch_down) {
		draggingMe = NULL; lastClick = d.lastClick; // Should be active:
		cpSpacePointQuery(spaces[0].space, lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_WALL); // Last arg is mask
		block_info *draggingBlock = draggingMe ? (block_info *)draggingMe->body->data : NULL;
        
		if (draggingBlock && draggingBlock->lock && !switch_info::button_down())
			draggingMe = NULL;
			
        d.dragging = draggingMe;
        d.draggingInto = draggingMeInto;
		
		if (draggingMe && draggingBlock) {
			if (!draggingBlock->siren) {
				const int NOTE_BASE = -19 + 2;
				const double halfstep = pow(2,1/12.0);
#if 1
#define SCALE 5
#define NOTES (SCALE*4)
				static bool haveGenerated = false;
				static int notes[NOTES];
				if (!haveGenerated) {
					haveGenerated = true;
					const int steps[SCALE] = {2, 2, 3, 2, 3};
					int num = NOTE_BASE;
					for(int c = 0 ; c < NOTES; c++) {
						notes[c] = num;
						num += steps[c%SCALE];
					}
				}
#if 0
				static int k = -1;
				k++;
#else
				unsigned int k = random();
#endif
				
				int coff = notes[k%NOTES];
#else
				int coff = random()%36 + NOTE_BASE;
#endif
				double f = pow(halfstep, -coff);
				draggingBlock->siren = am_flutter(f);
			}
			draggingBlock->siren->t2 = 0;
			draggingBlock->siren->period = 0;
		}
	}        
    if (d.dragging) {
		cpBody *body = d.dragging->body;
		cpBodySetMass(body, DRAG_MASS);
		block_info *b = (block_info *)body->data;
		b->dragging = true;
		b->wantp_s = at;
		b->updated_s = ticks;
		b->draggingInto = d.draggingInto;
    }
}

#ifdef TARGET_DESKTOP

void program_eventkey(SDL_Event &event) {
    for(eauto_iter i = event_automata.begin(); i != event_automata.end(); i++) {
        (*i)->input(event);
    }                    
	
	// DEBUG
#if 0
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_F5) {
		want = WDebugBlur;
		wantClearUi = true;
		program_interface();
	} else 
#endif
#if SELF_EDIT
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_F6) {
		wantClearUi = true;
		program_interface();
		newtype(new freetype_automaton());
	}
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == '\\') {
		spaces[0].player->spaces[0].body->p.x += 1.0;
	}
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == ']') {
		spaces[0].player->spaces[0].body->p.x -= 1.0;
	}
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == '=') {
		game_reset();
	}

#endif
}

// This is called when SDL gets a joystick event.
void program_eventjoy(SDL_Event &event) {
	for(eauto_iter i = event_automata.begin(); i != event_automata.end(); i++) {
        (*i)->input(event);
    }                    	
}

dragtouch globaldrag;

void program_eventmouse(SDL_Event &event) { // Probably you want to leave this as it is and use metamouse
    touch_type t = touch_move;
    if (event.type == SDL_MOUSEBUTTONDOWN)
        t = touch_down;
    if (event.type == SDL_MOUSEBUTTONUP)
        t = touch_up;
    program_metamouse(globaldrag, t, cpv(event.button.x, event.button.y));
}

#else

// Only used by mobile platforms at present-- Desktop main.cpp contains a more elaborate version
bool AttemptInterface(cpVect eventAt) {
    clickConnected = false;
    
    goOrtho(); // Is it okay to be putting display stuff in this file?
	void jcLoadIdentity(); jcLoadIdentity();
    cpVect at = screenToGL(eventAt.x, eventAt.y, 0);
    
    cpSpacePointQuery(workingUiSpace(), at, CP_ALL_LAYERS, CP_NO_GROUP, clicked, NULL);
    
    if (clickConnected)
        program_interface();
    
    return clickConnected;
}

typedef list<touch_rec>::const_iterator touches_iter;
struct touch_state : touch_rec {
    dragtouch d;
    touch_state() : touch_rec(0, cpvzero) {}
    touch_state(const touch_rec &r) : touch_rec(r) {}
};
typedef hash_map<touch_id, touch_state>::const_iterator alltouches_iter;
hash_map<touch_id, touch_state> allTouches; 

// This seems to me like it's more complicated than it should be. It used to be like four lines long
// but then I had to add a bunch of stuff to handle passing clicks over to interface.cpp
void program_eventtouch(const list<touch_rec> &touches, touch_type kind) {
    // Update alltouches
    for(touches_iter i = touches.begin(); i != touches.end(); i++) {
        switch(kind) {
            case touch_down: {
                touch_state &newstate = allTouches[i->tid];
                newstate = touch_state(*i);
                
                // If this touched the interface, censor it ahead of time
                if (AttemptInterface(i->at))
                    newstate.d.special = dragtouch::interface_special;
                else
                    program_metamouse(newstate.d, kind, i->at);
            } break;
            case touch_move: {
                touch_state &oldstate = allTouches[i->tid];
                oldstate.at = i->at;
                
                if (dragtouch::interface_special != oldstate.d.special)
                    program_metamouse(oldstate.d, kind, i->at);
            } break;
            case touch_up: case touch_cancel: {
                touch_state &oldstate = allTouches[i->tid];
                
                if (dragtouch::interface_special != oldstate.d.special)
                    program_metamouse(oldstate.d, kind, i->at);
                
                allTouches.erase( i->tid );
            } break;
        }
    }
}

#endif

// Real code starts here

// GAME MECHANICS

vector<player_handler *> player_handler::all_players;

// So for now I'm saying that when an "outside force" (like the player_handler) wants to modify something,
// it just modifies the actual body in the space. As long as something's "active" its p and v will be recorded
// rather than read.
void player_handler::setv(cpVect v) {
	player->lastv = v;
}

// W A S D == 2 0 3 1
#define KEYBOARD_SPEED 1.5

void player_handler::input(SDL_Event &event) {
	switch(event.type) {
		case SDL_KEYDOWN: case SDL_KEYUP: {
			if (-1 != jid) return; // BAIL OUT COMPLETELY
			for(int c = 0; c < KHKEYS; c++) {
				if (controls[c].key == event.key.keysym.sym) {
					if (event.type == SDL_KEYDOWN) {
						active[c] = true;
						active_frame[c] = true;
					} else { // Already checked this
						active[c] = false;
					}
				}
			}
		} break;			
		case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP: {
			if (event.jbutton.which != jid) return; // BAIL OUT COMPLETELY
			for(int c = 0; c < KHKEYS; c++) {
				if (controls[c].button == event.jbutton.button) {
					if (event.type == SDL_JOYBUTTONDOWN) {
						active[c] = true;
						active_frame[c] = true;
					} else { // Already checked this
						active[c] = false;
					}
				}
			}
		} break;
		case SDL_JOYHATMOTION: {
			if (event.jhat.which != jid) return; // BAIL OUT COMPLETELY
			for(int c = 0; c < JHATS; c++) {
				if (hat_at(c).hat == event.jhat.hat) {
					uint8_t value = event.jhat.value;
					active[4*c+0] = value & SDL_HAT_LEFT;
					active[4*c+1] = value & SDL_HAT_UP;
					active[4*c+2] = value & SDL_HAT_RIGHT;
					active[4*c+3] = value & SDL_HAT_DOWN;
					for(int d = 0; d < KHKEYS; d	++) // Since hat moves are "instant",
						active_frame[d] = active[d]; // Don't make hat positions sticky
				}
			}
		} break;
		case SDL_JOYAXISMOTION: {
			if (event.jaxis.which != jid) return; // BAIL OUT COMPLETELY
			for(int c = 0; c < JHAXES; c++) {
				if (axis_at(c).axis == event.jaxis.axis) {
					double value = double(event.jaxis.value)/(1<<15);
					if (axes_at[c] != ticks || value > axes[c]) {
						axes[c] = value;
						axes_at[c] = ticks;
					}
				}
			}
		} break;
	}	
}

void player_handler::tick() {
	if (game_halt || halted) return;
	
	// Velocity
	for(int a = 0; a < 2; a++) { // 2 axes		
		if (controls[a*4].is_axis()) { // If using stick
			cpVect v = cpv(axes[a*2],-axes[a*2+1]);
			
			if (0==a) { // Velocity
				if (cpvlength(v) > DPAD_THRESHOLD) {
					last_nonzero_v = v;
					setv(cpvmult(v, KEYBOARD_SPEED));
				} else {
					setv(cpvzero);
				}
			}
		} else { // If using buttons
			int e = -1; // number to rot by. Deduce diagonals:
			if (active_frame[a*4+1]) e = active_frame[a*4+2] ? 1 : (active_frame[a*4+0] ? 7 : 0);
			else if (active_frame[a*4+2]) e = active_frame[a*4+3] ? 3 : 2;
			else if (active_frame[a*4+3]) e = active_frame[a*4+0] ? 5 : 4;
			else if (active_frame[a*4+0]) e = 6;
			
			if (0==a) { // Velocity
				if (e >= 0) { // e-scale 0=top and goes clockwise. otherwise no
					last_nonzero_v = cpvforangle(M_PI/2 - 2.0*M_PI/8 * e);
					setv( cpvmult( last_nonzero_v, KEYBOARD_SPEED ) );
				} else {
					setv( cpvzero );
				}
			}
		}
	}
	
	for(int c = 0; c < KHKEYS; c++) {
		active_frame[c] = active[c];
	}			
}

int firstArgIsSwitch(cpArbiter *arb, cpSpace *space, void *data) {
	CP_ARBITER_GET_SHAPES(arb, a, b);
	switch_info *sw = (switch_info*)a->body->data;
	sw->updated = ticks;
	return sw->solid;
}

int triggerEnding(cpArbiter *arb, cpSpace *space, void *data) {
	want_ending = true;
}

void spaceinfo::setup_collisions() {
	cpSpaceAddCollisionHandler(space,
							   C_SEP, C_WALL,
							   NULL, firstArgIsSwitch, NULL, NULL, this);
	cpSpaceAddCollisionHandler(space,
							   C_BUTTON, C_PLAYER,
							   NULL, firstArgIsSwitch, NULL, NULL, this);
	cpSpaceAddCollisionHandler(space,
							   C_BUTTON, C_SLIDER,
							   NULL, firstArgIsSwitch, NULL, NULL, this);
	cpSpaceAddCollisionHandler(space,
							   C_EXIT, C_PLAYER,
							   NULL, triggerEnding, NULL, NULL, this);	
}

player_info *player_info::current_drawing; // When to clear this out?

bool player_info::drawing_player() {
	return player_info::current_drawing == this;
}