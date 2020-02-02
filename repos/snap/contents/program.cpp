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
		terminate_game();
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	// Default implementation does nothing.
}

void appendLevel(int time_off, double gravity) {
	int s = spaces.size();
  /* We first create a new space */
  spaces.push_back(spaceinfo(s, time_off));
  
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
	
	ERR("START LEVEL LOAD...");
	{
		char filename[FILENAMESIZE]; internalPath(filename, "levels.xml");
		load_levels_from(filename);
	}
	{
		DIR *dird = opendir(".");
		dirent *dir;
		while (dir = readdir(dird)) {
			if (strlen(dir->d_name) < 4 || strncmp( ".xml", dir->d_name + strlen(dir->d_name) - 4, 4 ))
				continue;
			load_levels_from(dir->d_name);
		}
		closedir(dird);
	}
	ERR("END LEVEL LOAD\n");					 
	
    program_reinit();
}

void
program_reinit(void)
{
	extern bool gl2;
	if (gl2 && GLEE_EXT_framebuffer_object)
		newtype(new logo_automaton());
	else
		newtype(new failure_automaton());
}

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

player_info * make_player(cpVect p, cpVect v, unsigned int mask, unsigned int color, unsigned int shotw) {
	player_info *info = new player_info();
	info->mask = mask;
	info->color = color;
	info->shotw = shotw;
	info->spaces.resize(spaces.size());
	// p and v get filled in later?
	
	for(int c = 0; c < spaces.size(); c++) {
		cpBody *newBody = cpBodyNew(5, INFINITY);
		newBody->data = info;
		newBody->p = p;
		newBody->v = v;
//		newBody->velocity_func = objectVelocityFunc;
//		newBody->position_func = objectPositionFunc;
		
		cpVect player_verts[] = SQUARE(PLAYER_RAD);
		
		cpShape *newShape = cpPolyShapeNew(newBody, 4, player_verts, cpvzero);
		newShape->e = li->p_e;
		newShape->u = li->p_u;
		newShape->collision_type = info->collision_type;
		newShape->layers = info->collision_interest;
		
		info->spaces[c].body = newBody;
		info->spaces[c].shapes.push_back(newShape);
	}

	return info;
}

bullet_info * make_bullet(cpVect p, cpVect v, player_info *owner) {
	bullet_info *info = new bullet_info();
	info->owner = owner;
	info->spaces.resize(spaces.size());
	
	for(int c = 0; c < spaces.size(); c++) {
		cpBody *newBody = cpBodyNew(5, INFINITY);
		newBody->data = info;
		newBody->p = p;
		newBody->v = v;
			
		cpShape *newShape = cpCircleShapeNew(newBody, BULLET_RAD, cpvzero);
		newShape->e = li->b_e;
		newShape->u = li->b_u; // SHOULD BE 0.8?
		newShape->collision_type = info->collision_type;
		newShape->layers = info->collision_interest;
		
		info->spaces[c].body = newBody;
		info->spaces[c].shapes.push_back(newShape);
	}
	
	return info;
}

drop_info * make_drop(cpVect p, unsigned int collision_type, int expire) {
	drop_info *info = new drop_info(collision_type);
	info->expire = expire;
	info->spaces.resize(spaces.size());
	
	for(int c = 0; c < spaces.size(); c++) {
		cpBody *newBody = cpBodyNew(5, INFINITY);
		newBody->data = info;
		newBody->p = p;
		newBody->v = cpvzero;
		
		cpVect drop_verts[] = SQUARE(DROP_RAD);
		cpShape *newShape = cpPolyShapeNew(newBody, 4, drop_verts, cpvzero);

		newShape->e = li->w_e;
		newShape->u = li->w_u; // SHOULD BE 0.8?
		newShape->collision_type = info->collision_type;
		newShape->layers = info->collision_interest;
		newShape->group = GROUP_PREDESTINED; // Means past selves will never spontaneously pick up health. Desirable?
		
		info->spaces[c].body = newBody;
		info->spaces[c].shapes.push_back(newShape);
	}
	
	return info;
}

forever now;

vector<level_idiom *> all_levels;
int chosen_level = 0, player_count = 2; // Note: player_count never changes, and I depend on this.
int wins[2] = {0,0};

// End an old game
void terminate_game() {
	for(int c = 0; c < player_handler::all_players.size(); c++)
		player_handler::all_players[c]->die();
	player_handler::all_players.clear();				
	
	game_halt = false; show_dead = 0; game_halted_at = 0;
	spaces.clear(); // TODO free memory
	now.moments.clear();
	li = new level_idiom(); // Must do this or drops will continue spawning with no space
	
	glClearColor(0,0,0,1); // AAAAAA
	
	nexttype(new level_automaton(chosen_level));	
}

// Start a new one
void game_reset() {
	game_halt = false; show_dead = 0; game_halted_at = 0;
	new_li(all_levels[chosen_level]);
	map_init(player_count);
}

void new_li(level_idiom *from) {
	delete li; li = from->clone();
}

void load_levels_from(string filename) {
	TiXmlDocument xml(filename);
	xml.LoadFile();
	TiXmlElement *rootxml = (TiXmlElement *)xml.IterateChildren("pack",NULL);
    if (!rootxml || rootxml->Type() != TiXmlNode::ELEMENT) return;
    TiXmlElement *cxml = NULL;
    while(1) {
        cxml = (TiXmlElement *)rootxml->IterateChildren("level", cxml);
        if (!cxml) break;
		if (cxml->Type() == TiXmlNode::ELEMENT) {
			level_idiom *i = new level_idiom();
			i->load(cxml);
			all_levels.push_back(i);
		}
	}
}

bool level_idiom::load_preview(string filename) {	
	ifstream f;
	//	f.exceptions( ios::eofbit | ios::failbit | ios::badbit ); // Absolutely don't, failure is legal
	f.open(filename.c_str(), ios_base::in | ios_base::binary);
	return f.read((char *)&preview[0], sizeof(preview));
}

void level_idiom::load(TiXmlElement *e) {
	name = e->Attribute("name");
	const char *file = e->Attribute("file");
	if (file) { // Should never be false except when in padded room...ds
        s = new block_slice(); s->consume(file, true);
		if (!s->pixel) {
			char filename[FILENAMESIZE]; internalPath(filename, file);
			s->consume(filename);
		}
		
		int xo = 0, yo = 0;
		if (s->width < PREVIEW_WIDTH) xo += (PREVIEW_WIDTH-s->width)/2;
		if (s->height < PREVIEW_HEIGHT) yo += (PREVIEW_HEIGHT-s->height)/2;
		
		for(int y = 0; y < s->height; y++) {
			for(int x = 0; x < s->width; x++) {
				unsigned int p = s->pixel[x][y];
				if (!(p&0xFF)) continue;
				if (PNG_FLOORTYPE(p)) {
					if (x < PREVIEW_WIDTH && y < PREVIEW_HEIGHT) {
						int px = xo + x, py = yo + y;
						preview[py*PREVIEW_WIDTH+px] = '*';
					}
				} else if (p == PNG_P1) {
					spawners[0].push_back(cpv(x,y));
				} else if (p == PNG_P2) {
					spawners[1].push_back(cpv(x,y));					
				} else if (p == PNG_DROP) {
					spawners[2].push_back(cpv(x,y));					
				} else {
					ERR("\nUnknown %x at %d,%d\n",p,x,y);
				}
			}
		}
		
		s->construct();
	}
	
	const char *pfile = e->Attribute("preview");
	if (pfile) {
		if (!load_preview(pfile)) {
			ERR("TRYING INTERNAL");
			char filename[FILENAMESIZE]; internalPath(filename, pfile);
			load_preview(filename);
		}
	}
	
	e->QueryIntAttribute("maxhealth", &maxhealth);
	e->QueryDoubleAttribute("std_zoom", &std_zoom);
	e->QueryDoubleAttribute("forever_length", &forever_length);
	e->QueryIntAttribute("snap_factor", &snap_factor);
	e->QueryDoubleAttribute("block_size", &block_size);
	e->QueryIntAttribute("have_camera_freedom", &have_camera_freedom);
	e->QueryIntAttribute("have_camera_limit", &have_camera_limit);
	e->QueryIntAttribute("repeat", &repeat);
	e->QueryIntAttribute("padded_room", &padded_room);
	e->QueryIntAttribute("immortal_bullets", &immortal_bullets);
	e->QueryDoubleAttribute("gravity", &gravity);
	
	e->QueryDoubleAttribute("w_e", &w_e); // Elasticity, friction
	e->QueryDoubleAttribute("p_e", &p_e);
	e->QueryDoubleAttribute("b_e", &b_e);
	e->QueryDoubleAttribute("w_u", &w_u);
	e->QueryDoubleAttribute("p_u", &p_u);
	e->QueryDoubleAttribute("b_u", &b_u);

	e->QueryDoubleAttribute("hd_p", &d_p[1]); // Drop stuff
	e->QueryDoubleAttribute("hd_a", &d_a[1]);
	e->QueryDoubleAttribute("ad_p", &d_p[0]);
	e->QueryDoubleAttribute("ad_a", &d_a[0]);
	e->QueryDoubleAttribute("sd_p", &d_p[2]);
	e->QueryDoubleAttribute("sd_a", &d_a[2]);	
	e->QueryIntAttribute("hd_q", &d_q[1]);
	e->QueryIntAttribute("ad_q", &d_q[0]);
	e->QueryIntAttribute("sd_q", &d_q[2]);

	
	{ const char *temp = e->Attribute("message"); if (temp) message = temp; }
}

level_idiom *level_idiom::clone() {
	level_idiom *i = new level_idiom();
	*i = *this;
	return i;
}

void map_init(int players) {
	spaces.clear(); // TODO free memory
	now.moments.clear();
	
	now.init(li->forever_length*FPS, int(li->forever_length*FPS)/li->snap_factor); // Casting to int cuz i want to!!
	
	for(int c = 0; c < now.period; c += now.snap)
		appendLevel(c, li->gravity);

    for(int s = 0; s < spaces.size(); s++)
		spaces[s].setup_collisions(); // Oh look if you resize spaces after doing this everything crashes later.

	extern cpRect arena;
	cpVect offset_p[2];
	if (players > 1) { // Unnecessary? Gets overwritten below.
		offset_p[0] = cpv(0,  0.5);
		offset_p[1] = cpv(0, -0.5);
	} else {
		offset_p[0] = cpvzero;
		offset_p[1] = cpvzero;
	}
		
	li->camera_visible = cpr(cpvzero, cpv(arena.rad.x/arena.rad.y,1));
	
	li->camera_nudge = li->camera_visible.inset(0.4);
	if (li->padded_room) { // Concerned by code duplication here.
		cpVect ul, br; // Used for padded room
		if (players > 1) { // This is too complicated! And it shouldn't be procedural anyway.
			ul = cpvmult(arena.rad, -0.5);
			ul = cpvadd(ul, cpv(-li->block_size,-li->block_size)); // This doesn't match the exact walls. FIXME
			br = cpvneg(ul);
		} else {
			ul = cpv(-1/aspect, -1);
			br = cpvneg(ul);
		}
		
		for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
			spaceinfo *s = &(*_s);
			cpRect room = li->camera_visible;

			for(int c = 0; c < 4; c++) {
				cpRect line = cpbounds(room.vert(c), room.vert((c+1)%4)).inset_fixed(-li->block_size/2);
				cpVect verts[4]; line.fill(verts);

				cpShape *newShape = cpPolyShapeNew(s->staticBody, 4, verts, cpvzero);
				cpSpaceAddStaticShape(s->space, newShape);
				newShape->e = li->w_e; newShape->u = li->w_u; newShape->collision_type = C_WALL; newShape->layers = C_WALL;
				newShape->group = GROUP_PREDESTINED;
			}
			
			li->camera_limit = li->camera_visible;
			li->have_camera_freedom = 0;
		}
	} else {
        block_slice *level = li->s;
		
		cpVect block_offset = cpv(-li->block_size*level->width/2,li->block_size*level->height/2);
		
		for(int c = 0; c < level->blocks.size(); c++) {
			block &b = level->blocks[c];
			cpRect r = cpbounds(cpv(b.x,-b.y),cpv(b.x+b.width,-b.y-b.height));
			r = r.scale(li->block_size).translate(block_offset);
			cpVect verts[4]; r.fill(verts);
			
			for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {	
				spaceinfo *s = &(*_s);
				cpShape *newShape = cpPolyShapeNew(s->staticBody, 4, verts, cpvzero);
				cpSpaceAddStaticShape(s->space, newShape);
				newShape->e = 0.8; newShape->u = 0.8; newShape->collision_type = C_WALL; newShape->layers = C_WALL;	
				newShape->group = GROUP_PREDESTINED;		
			}
		}
		
		for(int c = 0; c < 2; c++) {
			if (li->spawners[c].size())
				offset_p[c] = cpvadd( cpvadd( cpvmult( yflip(
											  li->spawners[c][random() % li->spawners[c].size()]
											  ), li->block_size), block_offset ),
									 cpv(li->block_size/2,-li->block_size/2) );
				//offset_p[c] = cpv(-offset_p[c].y, -offset_p[c].x); // VERY WORRISOME
		}
		
		li->camera_limit = cpbounds(cpvzero,cpv(level->width,-level->height)).scale(li->block_size).translate(block_offset);
	}

	int startspace = 0;
	int startframe = spaces[startspace].sid; // Complex way of saying "0"
	
	{
		player_info *p = make_player(offset_p[0], cpvzero, players > 1 ? 1 : 3, playerColor[0], playerShotw[0]);
		player_handler *k = new player_handler(p, startspace);
		k->load(defaults[0]);
		k->camera_zoom = li->std_zoom;
		k->push();
		now.moments[startframe].events.push_back(new object_lifetime(p,startframe));
		
		health_quantity *h = new health_quantity(p, startframe, li->starthealth(), 0); // Quantity events don't "fire" do they
		p->lifebar.push_back(h);
		
		spaces[0].visible = 3;
	}
	
	if (players > 1) {
		player_info *p = make_player(offset_p[1], cpvzero, 2, playerColor[1], playerShotw[1]);
		player_handler *k = new player_handler(p, startspace);
		k->load(defaults[1]);
		k->camera_zoom = li->std_zoom;
		k->push();
		now.moments[startframe].events.push_back(new object_lifetime(p, startframe));
		
		health_quantity *h = new health_quantity(p, startframe, li->starthealth(), 0); // Quantity events don't "fire" do they
		p->lifebar.push_back(h);
	}
	
	di->process = players > 1; // Right now all process does is inset the per-player screens
}

void event_automaton::die() {
	if (halted) return;
	halted = true;
    dying_event_auto.push_back(this);
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
	
	// Mark who player was at beginning of frame
	for(int _p = 0; _p < player_handler::all_players.size(); _p++) {
		player_handler::all_players[_p]->player_initially = player_handler::all_players[_p]->player;
	}
	
	// Clear out aftermath of last frame
	if (now.frame >= 0) { // Don't do this on time 0...
		for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
			spaceinfo *s = &(*_s);
			now.moments[s->time()].leave(s);
		}		
	}
	
	// Step frame
	now.frame++;
	
	// Good place to do this (?) -- any item drops?
	for(int c = 0; c < 3; c++) {
		if (li->d_p[c]) {
			int p = li->d_p[c]*FPS;
			int a = li->d_a[c]*FPS;
			if (now.frame >= a) {
				int f = (now.frame - a) % p;
				if (!f && li->spawners[2].size() && li->s) {
					int startframe = random() % now.period;
					// This was just cold copy and pasted from the offset_p set in the level build method.
					// This is a terrible place to do it and anyway the code shouldn't be duplicated.
					// Both should be moved to the point where the spawners array is being created, if possible.
					cpVect block_offset = cpv(-li->block_size*li->s->width/2,li->block_size*li->s->height/2); // DRY?
					cpVect at = cpvadd( cpvadd( cpvmult( yflip( // DRY!
															   li->spawners[2][random() % li->spawners[2].size()]
															   ), li->block_size), block_offset ),
									   cpv(li->block_size/2,-li->block_size/2) );
					
					ERR("GO!!! %d %d %d\n", c, f, startframe);
					
					unsigned int whatisthis[3] = {0, C_HEALTH, 0}; // others should eventually be C_AMMO, C_SNAP
					drop_info *d = make_drop(at, whatisthis[c], now.period);
					now.moments[startframe].events.push_back(new object_lifetime(d, startframe));
					
					now.lastdrop = startframe;
					now.lastdropat = now.frame;
					am_drop(3);
				}
			}
		}
	}
	
	// Fire up frame
	for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
		spaceinfo *s = &(*_s);
//		ERR("space %d at moment %d\n", s->sid, s->time());
		now.moments[s->time()].enter(s);
	}
	
	// Mechanics
	for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
		spaceinfo *s = &(*_s);
		int t = s->time();
		
		// if (!s->alive.size()) continue; // When to uncomment?
		for(lifetime_iter p = s->alive.begin(); p != s->alive.end(); p++) {
			p->second->be_alive_pre(s);
		}
				
		spaceRightNow = s;
		cpSpaceStep(s->space, DT);
		
		// Do this after step since step may cause things to shorten their lifetimes:
		for(lifetime_iter p = s->present.begin(); p != s->present.end(); p++) {
			f_lifetime *l = p->second;
			if (t + 1 >= l->ends()) { // If this is our last night on earth
				s->dying_this_frame.push_back(l); // Note it
				if (l->open_end) {
					int t2 = (t + 1)%now.period;
					now.moments[t2].events.push_back(l->reincarnate(s, s, t2)); // Insert into timeline at next frame
					l->open_end = false;
				}
			}
		}		
		
		// if (!s->alive.size()) continue; // When to uncomment?
		for(lifetime_iter p = s->alive.begin(); p != s->alive.end(); p++) {
			p->second->be_alive_post(s);
		}
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
}

void program_metamouse(dragtouch &d, touch_type t, cpVect at) {
	// Put your touch/click handler code here
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
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_F6) {
		wantClearUi = true;
		program_interface();
		newtype(new freetype_automaton());
	}
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

#define FIRE_RATE 10
void player_handler::setf(cpVect v) {
	if (ticks - last_fire >= FIRE_RATE) {
		spaceinfo *s = &spaces[sin]; // Insert into this space
		int t = (s->time() + 1)%now.period; // Consider the time one tick in the future
		bullet_info *bullet = make_bullet(player->spaces[sin].body->p, v, player); // Create bullet body (but don't insert)
		object_lifetime *bullet_life = new object_lifetime(bullet, t);
		now.moments[t].events.push_back(bullet_life); // Insert into timeline at next frame
		
		last_fire = ticks;
	}
}

// W A S D == 2 0 3 1
#define KEYBOARD_SPEED 1.5
#define FIRE_SPEED 3

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
			} else {
				if (cpvlength(v)>0.5) {
					setf(cpvmult(v, FIRE_SPEED));
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
			} else if (auto_aim) { // Auto fire (refactor later maybe?)
				if (active_frame[SPECIAL_FIRE_BUTTON]) {
					setf( cpvmult( last_nonzero_v, FIRE_SPEED ) );
				}
			} else if (e >= 0) { // Normal fire
				setf(cpvmult( cpvforangle(M_PI/2 - 2.0*M_PI/8 * e), FIRE_SPEED ));
			}
		}
	}
	
	if (active_frame[SNAP_BUTTON]) {
		int new_sin = (sin + spaces.size() - 1) % spaces.size();
		int t = spaces[sin].time();
		int t2 = spaces[new_sin].time();
		int t2plus = (t2 + 1)%now.period;
		
		int orig_sin = sin, orig_new_sin = new_sin; // I guess this changes in one of the reincarnates?
		
		// Destroy current player
		player->life->max = t; // End lifetime at frame we *just left* / are leaving
		player->life->open_end = false;
		spaces[sin].dying_this_frame.push_back(player->life);
		
		// Create new player
//		ERR("DIE IN %d AT %d, REINCARNATE IN %d AT %d\n", sin, t, new_sin, t2);
		f_lifetime *event = player->life->reincarnate(&spaces[sin], &spaces[new_sin], t2plus); // Notice: reincarnate *from* old space
		now.moments[t2plus].events.push_back(event); // Insert into timeline at next frame	
		
		// This casting is pretty awful.
		snap_animate *event2 = new snap_animate(t, (player_info *)((object_lifetime*)event)->watch, 0); // Notice: reincarnate *from* old space
		now.moments[t].events.push_back(event2); // Insert into timeline at next frame	
		if (spaces[orig_sin].visible & ~player->mask) event2->fire(&spaces[orig_sin]); // Will this screw up p/v?
		
		last_snap = ticks;
		am_snap(player->mask, orig_sin, orig_new_sin);
		active[SNAP_BUTTON] = false;
	}
	
	for(int c = 0; c < KHKEYS; c++) {
		active_frame[c] = active[c];
	}			
}

void forever::init(int _period, int _snap) {
	period = _period;
	snap = _snap;
	frame = -1;
	lastdrop = -1; lastdropat = 0;
	moments.resize(period);
}

void moment::enter(spaceinfo *s) {
	for(int c = 0; c < events.size(); c++) {
		if (events[c]->canon)
			events[c]->fire(s);
	}
}

// UNACCEPTABLE, FIX
void moment::leave(spaceinfo *s) {
	for(int c = 0; c < s->dying_this_frame.size(); c++)
		s->dying_this_frame[c]->halt(s);
	s->dying_this_frame.clear();
}

void f_event::make_depend(f_event *on, int at, int after) {
	on->children.push_back(depend(this, at, after));
#if TRACK_EVENT_PARENTS
	parents.push_back(depend(on, at, after));
#endif
}

// Wow, this is sort of ugly.
void f_event::set_canon(bool is, int after) {
	if ( after < 0 ? (is==canon) : (after >= ((f_lifetime*)this)->max) ) // If redundant do nothing
		return;
	for(int c = 0; c < children.size(); c++) {
		if (after < 0 || children[c].at > after)
			children[c].on->set_canon(is, children[c].after);
	}
	if (after >= 0)
		((f_lifetime*)this)->max = after; // FIXME: Someday "is" will be called with true and this won't work
	else
		canon = is;
}

void f_lifetime::fire(spaceinfo *s) {
	s->present[this] = this;
}

f_lifetime *f_lifetime::reincarnate(spaceinfo *, spaceinfo *, int t) { return new f_lifetime(t, now.period); }

void f_lifetime::halt(spaceinfo *s) {
	s->present.erase(this);
	s->alive.erase(this); // TODO: Where to do this?
}

void object_lifetime::remove_free_will(int at) {
	watch->spaces[at].body->velocity_func = objectVelocityFunc;
	watch->spaces[at].body->position_func = objectPositionFunc;
	for(int c = 0; c < watch->spaces[at].shapes.size(); c++)
		watch->spaces[at].shapes[c]->group = GROUP_PREDESTINED;
}

void object_lifetime::fire(spaceinfo *s) {
	f_lifetime::fire(s);
	
	if (!watch->p.size()) {
		s->alive[this] = this;
	} else {
		remove_free_will(s->sid); // Sometimes redundant. WHO CARES
	}
	
	live_connector &connect = watch->spaces[s->sid];
	if (watch->p.size()) { // If this is a "respawn", our body will have a stupid initial value.
		connect.body->p = watch->p[0]; // Fix it real quick before we do any collision detection.
		connect.body->v = watch->v[0]; // FIXME There must be a better way/time of doing this.
	}
	cpSpaceAddBody(s->space, connect.body);
	for(int c = 0; c < connect.shapes.size(); c++) {
		cpSpaceAddShape(s->space, connect.shapes[c]);
		cpSpaceRehashShape(s->space, connect.shapes[c]); // Only needed if watch->p was modified above
	}
	connect.attached = true;
	if (!soft_launch)
		watch->spawn(s);
}

void object_lifetime::halt(spaceinfo *s) {
	f_lifetime::halt(s);
	live_connector &connect = watch->spaces[s->sid];
	cpSpaceRemoveBody(s->space, connect.body);
	for(int c = 0; c < connect.shapes.size(); c++)
		cpSpaceRemoveShape(s->space, connect.shapes[c]);
	connect.attached = false;
}

void object_lifetime::be_alive_pre(spaceinfo *s) {
	cpBody *body = watch->spaces[s->sid].body;

	if (watch->collision_type == C_PLAYER) {
		cpVect desv = ((player_info *)watch)->lastv;
		cpVect newv = cpvmult( cpvsub(desv,body->v), 1/DT ); // Very alarmed by this line.
		
		if (li->gravity) { // So that we don't cancel out gravity by "returning to zero"
			if (!desv.x && !desv.y)
				newv = cpvzero;
		}
		
		cpBodyResetForces(body);
//		ERR("v %lf,%lf\n", newv.x,newv.y);
		cpBodyApplyForce(body, newv, cpvzero);
	}
}

void object_lifetime::be_alive_post(spaceinfo *s) {
	cpBody *body = watch->spaces[s->sid].body;
	watch->v.push_back(body->v);
	watch->p.push_back(body->p);
	
	// Assuming open_end means "we were not just killed by snapping".
	// Worried about this. What if you snap and this happens at once?
	// I guess you'll wrap next frame which won't be so bad.
	if (li->repeat && open_end) { 
		cpFloat t;
		int loop_x = 0, loop_y = 0;
		int time = s->time(); // t/time... ew
		if (li->repeat & 1) { // At what point do I hit DRY?
			t = body->p.x-li->camera_limit.r(); if (t>0) loop_x = 1;
			t = li->camera_limit.l()-body->p.x; if (t>0) loop_x = -1;
		}
		if (li->repeat & 2) {
			t = body->p.y-li->camera_limit.u(); if (t>0) loop_y = 1;
			t = li->camera_limit.d()-body->p.y; if (t>0) loop_y = -1;
		}
		if ((loop_x || loop_y) && time+1<now.period) { // Final clause should never trigger but I'm feeling paranoid.
			max = time+1; // Really would be better if max was time and replacement entry point was time, but... eh.
			open_end = false;
			s->dying_this_frame.push_back(this);
			
			object_lifetime * replacement = (object_lifetime *)reincarnate(s, s, time+1);
			cpVect &p = replacement->watch->spaces[s->sid].body->p;
			cpVect loopsize = li->camera_limit.size();
			cpVect loopoffset = cpv(loopsize.x * -loop_x, loopsize.y * -loop_y);
			p = cpvadd(p, loopoffset); // Offset against loop dir
			if (watch->collision_type == C_PLAYER) {
				for(int c = 0; c < player_handler::all_players.size(); c++) {
					player_handler *p = player_handler::all_players[c];
					if (p->player == replacement->watch) {
						ERR("%d, %d: LOOP!\n", c, ticks);
						p->camera_off = cpvadd(p->camera_off, loopoffset);
					}
				}
			}
			now.moments[time+1].events.push_back(replacement);
		}
	}
}

f_lifetime *object_lifetime::reincarnate(spaceinfo *from, spaceinfo *to, int t) {
	f_lifetime *l = new object_lifetime(watch->reincarnate(from, to, t), t); l->soft_launch = true; return l;
}

void audio_event::fire(spaceinfo *s) {
	if (s->visible) {
		bgaudio *b = new bgaudio( n->clone(s->visible), false, true);
		b->insert();
	}
}

void animate_lifetime::fire(spaceinfo *s) {
	f_lifetime::fire(s);
	if (s->visible && n) {
		bgaudio *b = new bgaudio( n->clone(s->visible), false, true);
		b->insert();
	}
}

f_quantity *f_quantity::reincarnate(int occur, int new_increment, bool contemporary) {
	f_quantity *q = new f_quantity(occur, value+new_increment, new_increment, next); next = q; return q;
}

// Best way to do this?
void f_quantity::propagate() { // Assume we are pre-incremented.
	int current = value;
	for(f_quantity *at = next; at; at = at->next) {
		if (at->canon) {
			current += at->increment;
			at->value = current;
			ERR("THISONE: %d = %d\n", at->increment, at->value);
		}
	}
}

f_quantity *health_quantity::reincarnate(int occur, int new_increment, bool contemporary) {
	health_quantity *q = new health_quantity(owner, occur, value+new_increment, new_increment, next); next = q; return q;
}

// DO repeat yourself cuz like this is a performance risk.
void health_quantity::propagate() { // Assume we are pre-incremented.
	int current = value;
	int diedat = -1;
	if (value <= 0) { // This gets a little DRY-y
		diedat = occur;
		value = 0;
	}
	if (value > li->maxhealth) {
		value = li->maxhealth;
	}	
	for(f_quantity *at = next; at; at = at->next) {
		if (diedat >= 0) {
			at->value = 0;
		} else if (at->canon) {
			current += at->increment;
			at->value = current;
			if (at->value <= 0) {
				at->value = 0;
				diedat = at->canon;
			}
			if (at->value > li->maxhealth) {
				at->value = li->maxhealth;
			}
		}
	}
	if (diedat >= 0) {
		gameover_event *go_event = new gameover_event(diedat, owner->mask);
		now.moments[diedat].events.push_back( go_event );		
	}
}

void health_quantity::set_canon(bool is, int after) {
	bool canon_was = canon;
	f_event::set_canon(is,after);
	if (canon_was != canon) { // Undo
		value += (increment * (is?1:-1));
		propagate();
	}
}

f_lifetime *kash_animate::reincarnate(spaceinfo *from, spaceinfo *to, int t) {
	kash_animate *l = new kash_animate(t, p, frame(from), NULL); // rebirth doesn't get a sound effect. // Use max instead of frame()?
	l->make_depend(this, -1, -1); // Someday must fix the "reincarnates depend" problem
	return l;
}

f_lifetime *snap_animate::reincarnate(spaceinfo *from, spaceinfo *to, int t) {
	snap_animate *l = new snap_animate(t, follow, frame(from)); // rebirth doesn't get a sound effect. // Use max instead of frame()?
	l->make_depend(this, -1, -1); // Someday must fix the "reincarnates depend" problem
	return l;
}

void gameover_event::fire(spaceinfo *) {
	game_halt = true;
	game_halted_at = ticks;
	show_dead |= deathmask;
	for(int c = 0; c < 2; c++) // CLUMSY/MAGIC
		if (deathmask & (1<<c))
			wins[!c]++;
	newtype(new restart_automaton());
}

int bulletHitsWall(cpArbiter *arb, cpSpace *space, void *data) {
	CP_ARBITER_GET_SHAPES(arb, a, b);
	bullet_info *bullet = (bullet_info*)a->body->data;
	spaceinfo *s = (spaceinfo *)data;
	
	if (li->immortal_bullets)
		return true;
	
	bullet->land(s,NULL);
	return false;
}

int bulletHitsPlayer(cpArbiter *arb, cpSpace *space, void *data) {
	CP_ARBITER_GET_SHAPES(arb, a, b);
	bullet_info *bullet = (bullet_info*)a->body->data;
	player_info *player = (player_info*)b->body->data;
	spaceinfo *s = (spaceinfo *)data;
	
#if 0
	// This doesn't work for the same reason bullet knockback doesn't work.
	if (li->immortal_bullets && bullet->owner->color == player->color) {
		return true;
	}
#endif
	
	if (bullet->owner != player) {
		bullet->land(s,player);
	}
	return false;
}

int playerHitsDrop(cpArbiter *arb, cpSpace *space, void *data) {
	CP_ARBITER_GET_SHAPES(arb, a, b);
	drop_info *drop = (drop_info*)a->body->data;
	player_info *player = (player_info*)b->body->data;
	spaceinfo *s = (spaceinfo *)data;

	// Too much of this is pure copied from bullet_info::land.
	int prefmax = (s->time()+1)%now.period;
	bool wrap = true;
	if (prefmax && prefmax < drop->life->max) { // TODO: Deal with the "wrap" problem more correctly. Pass to child?
		drop->life->set_canon(false, prefmax);
		wrap = false;
	}
	drop->life->open_end = false;
	am_pickup(s->visible);
	
	for(health_iter i = player->lifebar.begin(); i != player->lifebar.end(); i++) {
		health_quantity *me = *i;
		health_iter _next = i; _next++; 
		health_quantity *next = _next != player->lifebar.end() ? *_next : NULL;
		
		if (!next || (prefmax >= me->occur && prefmax < next->occur)) {
			health_quantity *new_health = (health_quantity *)me->reincarnate(prefmax, li->d_q[1], true); // Note: Health only at this point.
			//				ERR("New health? %d (%d <= %d < %d)\n", new_health->value, me->occur, prefmax, next?next->occur:-1);
			player->lifebar.insert(_next, new_health);
			new_health->make_depend(drop->life,prefmax,-1);
			new_health->propagate();
			break;
		}
	}	
	
	return false;
}

void spaceinfo::setup_collisions() {									
	cpSpaceAddCollisionHandler(space,
							   C_BULLET, C_WALL,
							   NULL, bulletHitsWall, NULL, NULL, this);	
	cpSpaceAddCollisionHandler(space,
							   C_BULLET, C_HEALTH,
							   NULL, bulletHitsWall, NULL, NULL, this);		
	cpSpaceAddCollisionHandler(space,
							   C_BULLET, C_PLAYER,
							   NULL, bulletHitsPlayer, NULL, NULL, this);
	cpSpaceAddCollisionHandler(space,
							   C_HEALTH, C_PLAYER,
							   NULL, playerHitsDrop, NULL, NULL, this);	
}

// Do I give "too much" information here?
live_info *player_info::reincarnate(spaceinfo *from, spaceinfo *to, int t) {
	cpBody *body = spaces[from->sid].body;
	player_info *p = make_player(body->p, body->v, mask, color, shotw);
	
	f_quantity *new_health = health()->reincarnate(t, 0, false); // No increment, not contemporary
	p->lifebar.push_back( (health_quantity *)new_health );
	
	for(int c = 0; c < player_handler::all_players.size(); c++) {
		player_handler *handler = player_handler::all_players[c];
		if (handler->player == this) {
			handler->player = p; 
			handler->sin = to->sid;
			from->visible &= ~handler->player->mask;
			to->visible |= handler->player->mask;
		}
	}
	return p;
}

player_info *player_info::current_drawing; // When to clear this out?

bool player_info::drawing_player() {
	return player_info::current_drawing == this;
}

live_info *bullet_info::reincarnate(spaceinfo *from, spaceinfo *, int) {
	cpBody *body = spaces[from->sid].body;
	return make_bullet(body->p, body->v, owner);
}

void bullet_info::spawn(spaceinfo *s) {
	if (s->visible)
		am_pskew(s->visible, owner->shotw);
}

void bullet_info::land(spaceinfo *s, player_info *victim) {
	// This will be checked after be_alive.
	// TODO: In future do this with a turnoff, not max.
	int prefmax = (s->time()+1)%now.period;
	bool wrap = true;
	if (prefmax && prefmax < life->max) { // TODO: Deal with the "wrap" problem more correctly. Pass to child?
		life->set_canon(false, prefmax);
		wrap = false;
	}
	life->open_end = false;
	cpBody *body = spaces[s->sid].body;
	kash_animate *hit_event = new kash_animate(prefmax, cpvadd(body->p,cpvmult(body->v,DT)), 0, get_kash(0)); // DRY DRY DRY
	now.moments[prefmax].events.push_back( hit_event );	
	hit_event->make_depend(life, prefmax, -1);
	
	if (victim && !wrap) {
//		ERR("Iam %p Ihave %d\n", this, victim->lifebar.size());
		for(health_iter i = victim->lifebar.begin(); i != victim->lifebar.end(); i++) {
			health_quantity *me = *i;
			health_iter _next = i; _next++; 
			health_quantity *next = _next != victim->lifebar.end() ? *_next : NULL;
			
			if (!next || (prefmax >= me->occur && prefmax < next->occur)) {
				health_quantity *new_health = (health_quantity *)me->reincarnate(prefmax, -BULLET_DAMAGE, true);
//				ERR("New health? %d (%d <= %d < %d)\n", new_health->value, me->occur, prefmax, next?next->occur:-1);
				victim->lifebar.insert(_next, new_health);
				new_health->make_depend(life,prefmax,-1);
				new_health->propagate();
				break;
			}
		}
	}
}

drop_info::drop_info(int whatami) : live_info(whatami, C_PLAYER | C_BULLET), expire(0) {}

void drop_info::spawn(spaceinfo *s) { // Just a convenient place to check...
	ERR("DROPSPAWN!!! %d\n", s->sid);
	if (life->max > life->occur + expire)
		life->max = life->occur + expire;
	if (s->visible)
		am_drop2(s->visible);
}

live_info *drop_info::reincarnate(spaceinfo *from, spaceinfo *, int) {
	cpBody *body = spaces[from->sid].body;
	return make_drop(body->p, collision_type, expire);
}

int spaceinfo::time() {
	return (now.frame+time_off)%now.period;
}