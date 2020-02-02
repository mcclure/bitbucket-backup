// BUSINESS LOGIC

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andrew McClure
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

#include "chipmunk.h"
#include "color.h"
#include "program.h"

#define GRAVITY cpv(0.0f, -9.8/2)

vector<spaceinfo> spaces;

cpSpace *uiSpace = NULL;
vector<ContainerBase *> columns;
bool wantClearUi = false;
WantNewScreen want = WNothing;

list<automaton *> automata;
vector<auto_iter> dying_auto;

list<environment *> environments;
environment *base_environment = NULL;

bool running = false;

// ------------- Simple objects ---------------

eobject::eobject(int _x1, int _y1, int _w, int _h, unsigned int _ec) : x1(_x1), y1(_y1), w(_w), h(_h), ec(_ec) {}
// Right now, one false = stop calling me
bool eobject::check_corners(callback *c) {
    return c->check(x1, y1)
        && c->check(x2(), y1)
        && c->check(x2(), y2())
        && c->check(x1, y2());
}

// Just for testing the graphics chain.
void test_box_environ::input(SDL_Event &event) {
    SDLKey &key = event.key.keysym.sym;
    bool arrow = false;
    int which = 0;
    
    // ARROW KEYS
    
	for(int c = 0; c < 4; c++) {
		if (key == arrow_trigger[c]) {
			arrow = true;
			which = c;
		}
	}
    
    if (arrow) {
        arrows[which] = event.type == SDL_KEYDOWN;    
    }
	
	if (key == SDLK_SPACE && event.type == SDL_KEYDOWN) { jump_pressed = true; }
}

void test_box_environ::tick() {
    if (ticks-last_moved<3)
        return;
    last_moved = ticks;
    if (arrows[0]) y--;
    if (arrows[1]) y++;
    if (arrows[2]) x++;
    if (arrows[3]) x--;
}

void level_environment::init() {
    for(int y = 0; y < l->height; y++) {
        for(int x = 0; x < l->width; x++) {
            unsigned int &pixel = l->pixel[x][y];
            if (pixel == EC_PLAYER) {
                pixel = 0xFFFFFF00;
                player_eo *_player = new player_eo(x, y);
                objects.push_back(_player);
                if (!player)
                    player = _player;
            }
        }
    }
}

struct collide_callback : public eobject::callback {
	level_environment *e; int xo, yo; unsigned int match;
	collide_callback(level_environment *_e, int _xo = 0, int _yo = 0, unsigned int _match = EC_WALL)
		: e(_e), xo(_xo), yo(_yo), match(_match) {}
	virtual bool check(int x, int y) {
		x += xo; y += yo;
		return e->l->inside(x,y) && !(e->l->pixel[x][y] == match);
	}
};

void level_environment::tick() {
    if (ticks-last_moved<3)
        return;
	
	if (player->dead) return;
    
    if (arrows[2]) { 
		collide_callback here(this, 1,0);
		if (player->check_corners(&here)) {
			x++; player->x1++; 
			if (player->run_start < 0) player->run_start = ticks;
		} else player->run_start = -1;
		player->xflip = false; // We want to turn even if we're not moving.
	} else if (arrows[3]) { 
		collide_callback here(this, -1,0);
		if (player->check_corners(&here)) {
			x--; player->x1--; 
			if (player->run_start < 0) player->run_start = ticks; 
		} else player->run_start = -1;
		player->xflip = true;
	} else { 
        player->run_start = -1;
        test_box_environ::tick(); // up/down pan
    }
	
	if (jump_pressed) {
		if (player->grounded) {
			player->grounded = false;
			player->impetus = 6;
		}
		jump_pressed = false;
	}
	
	{ // Attempt jump/fall
		int jumpdir = -1;
		if (player->impetus > 0) { player->impetus--; jumpdir = 1; }
		collide_callback here(this, 0,jumpdir);
		if (player->check_corners(&here)) {
			player->y1 += jumpdir;
		} else if (jumpdir < 0) {
			player->grounded = true;
		}
	}
	
	{ // Attempt drown/die
		collide_callback here(this, 0, 0, EC_DEEPWATER); // Collide should be more sophisticated
		if (!player->check_corners(&here)) {
			player->dead = true;
			extern int tapping; tapping = 0; // DON'T DO IT LIKE THIS
		}
	}
    
    last_moved = ticks;
}

// /------ Leftover from drumcircle/angels ------\

cpVect lastClick = cpvzero; // FIXME
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

// \------ begin display state ------/

void appendLevel(double depth) {
	int s = spaces.size(); // Is any of this even used?
  /* We first create a new space */
  spaces.push_back(spaceinfo());
  spaces[s].depth = depth;  
  
	spaces[s].space = cpSpaceNew();
    
    spaces[s].space->damping = 0.01;
	
	/* Next, you'll want to set the properties of the space such as the
     number of iterations to use in the constraint solver, the amount
     of gravity, or the amount of damping. In this case, we'll just
     set the gravity. */
	spaces[s].space->gravity = cpvzero;
	
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
    for(int c = 0; c < DEBUG_BOOLS_COUNT; c++) debug_bools[c] = false;
    
    cpInitChipmunk();
	uiSpace = cpSpaceNew();  
    
    program_reinit();
	
	want = WMainMenu;
}

bool arrows[4] = {false, false, false, false}; //   debug
const char *debug_what[9] = {"Resolution", "Gon", "Pack", "n/a", "n/a", "n/a", "Rings", "Per arm offset", "n/a"};
int debug_values[9] = {DEBUG_VALUE_BEND_DEFAULT, DEBUG_VALUE_GON_DEFAULT, DEBUG_VALUE_PACK_DEFAULT,
        0, 0, 0,
        DEBUG_VALUE_OUTWARD_DEFAULT, DEBUG_VALUE_PERARM_DEFAULT, 0};
int debug_floating[3] = {0,0,0};
float debug_floats[3] = {0,0,0};
bool debug_bools[DEBUG_BOOLS_COUNT];
int debug_masterint = -1;
int debug_stamp = 0, debug_stampres = 0;
void *debug_stamp_id = NULL;
#define MOSTBUTTONS 5
cpVect debug_mouse[MOSTBUTTONS];
bool have_debug_mouse[MOSTBUTTONS] = {false, false, false, false, false};

#define NO_BACKTRACK 1
cpVect orient; // "Mouse" (or analog) direction

// Not much of a reason to run this at startup...
void
program_reinit(void)
{
}

void program_clear(void) {
	for(environ_iter i = environments.begin(); i != environments.end(); i++)
        delete *i;
	environments.clear();
	base_environment = NULL;
	
	extern blitter *blit; // Ugh...
	if (blit) {
		delete blit;
		blit = NULL;
	}
	
    for(auto_iter i = automata.begin(); i != automata.end(); i++)
        delete *i;
	automata.clear();
}

void automaton::tick() {
    if (age() > 0) {
        frame++;
    }
    if (frame > rollover) {
        state++;
        rollover = 0;
    }
}

void automaton::die() {
    dying_auto.push_back(anchor);
}

// The default display() calls this once per framedraw.
void
program_update(void)
{
    ticks++;
    
    for(int c = 0; c < 3; c++) {
        debug_floats[c] += debug_floating[c];
        if (debug_floating[c]) { // Nonsense
//            BANNER(FPS/10) << "Floating[" << c << "] = " << debug_floats[c];
            ERR("floating[%d] = %lf\n", c, debug_floats[c]);
        }
    }
    
    // Environment management
    for(environ_iter i = environments.begin(); i != environments.end(); i++) {
        (*i)->tick();
    }                
    
    // Automata management
    for(auto_iter i = automata.begin(); i != automata.end(); i++) {
        (*i)->tick();
    }            
    if (!dying_auto.empty()) {
        for(int c = 0; c < dying_auto.size(); c++) {
            automaton *a = *dying_auto[c];
            automata.erase(dying_auto[c]);
            delete a;
        }
        dying_auto.clear();
    }
    
  /* Collision detection isn't amazing in Chipmunk yet. Fast moving
     objects can pass right though each other if they move too much in a
     single step. To deal with this, you can just make your steps
     smaller and cpSpaceStep() several times. */
}

void program_metamouse(dragtouch &d, touch_type t, cpVect at) {
	// I suggest keeping this little 4-line sigil at the top so that the interface library will work:
	if (t == touch_down && interface_attempt_click(at, d.button)) // Let the interface library look at the event first
		d.special = dragtouch::interface_special;
	if (d.special == dragtouch::interface_special) // Once a touch has been marked as part of the interface library, ignore it
		return;		
}

#ifdef TARGET_DESKTOP

static void eventkey_debugvalue(SDL_Event &event) {
    int dv = 0, dr = 0;
    SDLKey &key = event.key.keysym.sym;

    if (event.type == SDL_KEYDOWN) { // DEBUG KEYS
        if (key == 'q') { dv = 0; dr = -1; }
        if (key == 'e') { dv = 0; dr = 1; }
        if (key == 'a') { dv = 1; dr = -1; }
        if (key == 'd') { dv = 1; dr = 1; }
        if (key == 'z') { dv = 2; dr = -1; }
        if (key == 'c') { dv = 2; dr = 1; }
        
        if (SDL_GetModState() & KMOD_LSHIFT) dv += 3;
        if (SDL_GetModState() & KMOD_LCTRL) dv += 6;
    }    
    
    if (dr) {
        debug_values[dv] += dr;
        ERR("DEBUG %s %d:\t%d\t= %d\n", debug_what[dv], dv, dr, debug_values[dv]);
        if (DEBUG_VALUE_REDRAW_ON(dv))
            program_reinit();
    }    
}

// This is called when SDL gets a key event. Clear out and replace with your own code
// Not to be confused with test_box_environ::input
static void eventkey_fly(SDL_Event &event) {
    //debug
    bool arrow = false;
    int which = 0;
    int fv = 0, fr = 0;
    
    SDLKey &key = event.key.keysym.sym;
    
    // ARROW KEYS
    
    if (key == SDLK_UP) { // DEBUG-- SHOULDN'T BE KEPT AROUND
        arrow = true; which = 1;
    }
    if (key == SDLK_DOWN) {
        arrow = true; which = 0;
    }
    if (key == SDLK_LEFT) {
        arrow = true; which = 3;
    }
    if (key == SDLK_RIGHT) {
        arrow = true; which = 2;
    }
    eventkey_debugvalue(event);
    if (event.type == SDL_KEYDOWN) { // DEBUG KEYS        
//        if (key == '[') { debug_off_mo = moident; debug_off_rot = moident; debug_off_dr = 0; rollstep = 0; lights.front()->at = MoFromTranslate( cpv(0.8,0)); }
//        if (key == ']') { debug_trip_poison = !debug_trip_poison; }
        if (key == '\\') { paused = !paused; }
//      if (key == ']') { debug_off_mo = MoCompose(debug_off_mo, MoFromTranslate(cpvrotate(loaded->dist, cpvforangle(debug_floats[1])))); }
        
        if (key >= '0' && key <= '9') { // NUMBERS
            int pressed = key - '0';
            if (SDL_GetModState() & KMOD_LCTRL) { // Switch stamps
                if (pressed == debug_masterint)
                    debug_masterint = -1;
                else
                    debug_masterint = pressed;
                BANNER(TPF) << "Masterint " << debug_masterint;
            } else {
                debug_bools[key - '0'] = !debug_bools[key - '0'];
                for(int c = 0; c < DEBUG_BOOLS_COUNT; c++)
                    ERR((debug_bools[c] ? "Y":"N"));
                ERR("\n");
            }
        }
    }
    
    // DEBUG FLOATS
    if (key == 'u') { fv = 0; fr = -1; }
    if (key == 'o') { fv = 0; fr = 1; }
    if (key == 'j') { fv = 1; fr = -1; }
    if (key == 'l') { fv = 1; fr = 1; }
    if (key == 'm') { fv = 2; fr = -1; }
    if (key == '.') { fv = 2; fr = 1; }
    
    if (fr) {
        if (event.type == SDL_KEYUP)
            fr = 0;
        debug_floating[fv] = fr;
    }
    
    if (arrow)
        arrows[which] = event.type == SDL_KEYDOWN;    
}

void eventkey_game(SDL_Event &event) {
    for(environ_iter i = environments.begin(); i != environments.end(); i++) {
        (*i)->input(event);
    }                    
}

void program_eventkey(SDL_Event &event) {
    SDLKey &key = event.key.keysym.sym;

    // MODE SWITCH
    if (event.type == SDL_KEYDOWN && key >= SDLK_F1 && key <= SDLK_F12) {
        int mode = key - SDLK_F1;
        if (mode < run_max) {
            const char *runs[run_max] = {"Flythrough", "Game"};
            run_mode = (RunMode)mode;
            { BANNER(FPS*2) << runs[mode]; }
        }
    }
    switch (run_mode) {
        case run_fly:
            eventkey_fly(event);
            break;
        case run_game:
            eventkey_game(event);
    }
}

// This is called when SDL gets a joystick event.
void program_eventjoy(SDL_Event &event) {
}

dragtouch globaldrag;

void program_eventmouse(SDL_Event &event) {
	touch_type t = touch_move;
	cpVect event_at = cpv(event.button.x, event.button.y);
	
    if (event.type == SDL_MOUSEBUTTONDOWN)
        t = touch_down;
    if (event.type == SDL_MOUSEBUTTONUP)
        t = touch_up;
	
	globaldrag.button = event.button.button;
	program_metamouse(globaldrag, t, event_at);	
}

#else

typedef list<touch_rec>::const_iterator touches_iter;
struct touch_state : touch_rec {
    dragtouch d;
    touch_state() : touch_rec(0, cpvzero) {}
    touch_state(const touch_rec &r) : touch_rec(r) {}
};
typedef hash_map<touch_id, touch_state>::const_iterator alltouches_iter;
hash_map<touch_id, touch_state> allTouches; 

void program_eventtouch(const list<touch_rec> &touches, touch_type kind) {
    // Update alltouches
    for(touches_iter i = touches.begin(); i != touches.end(); i++) {
        switch(kind) {
            case touch_down: {
                touch_state &newstate = allTouches[i->tid];
                newstate = touch_state(*i);
                program_metamouse(newstate.d, kind, i->at);
            } break;
            case touch_move: {
                touch_state &oldstate = allTouches[i->tid];
                oldstate.at = i->at;
                program_metamouse(oldstate.d, kind, i->at);
            } break;
            case touch_up: case touch_cancel: {
                touch_state &oldstate = allTouches[i->tid];
                program_metamouse(oldstate.d, kind, i->at);
                allTouches.erase( i->tid );
            } break;
        }
    }
}

#endif

void handleDrags() {
#ifdef TARGET_DESKTOP
    {
        dragtouch &d = globaldrag;
#else
    for (alltouches_iter i = allTouches.begin(); i != allTouches.end(); i++) {
        const dragtouch &d = i->second.d;
#endif
        if (d.dragging) {
            cpVect force = cpvmult(cpvsub(d.lastClick, d.dragging->body->p), FPS);
            //		  ERR("V %f, %f\n", mouseBody->p.x, mouseBody->p.y, force.x, force.y);
            d.dragging->body->v = force;		  
        }                
    }
}
	
// The "this is a good time to modify your uiSpace" function. Main.cpp calls this once at the beginning of the main
// loop and once every time a button is successfully clicked. This exists because it's unsafe to modify the interface
// from inside of a ControlBase click() handler, so instead click() handlers can stash some state that this function
// then picks up when main.cpp automatically calls it after the click is completed...
void program_interface() {
	WantNewScreen wantAfterwards = WNothing; // "want" will be cleared with this at the end of the function
	
#ifdef TARGET_DESKTOP
	if (wantClearUi) {
		for(int c = 0; c < columns.size(); c++)
			if (columns[c])
				delete columns[c];
		
		columns.clear();
		
		resetScrollmax();
		wantClearUi = false;
	}
#endif
	
	switch (want) {	
		case WMainMenu: {
            ContainerBase *checkboxes = new ContainerBase(uiSpace, CMID);
			columns.push_back(checkboxes);
			
            checkboxes->add( new ControlBase("Impression") );
			checkboxes->add( new MiscButton("Tank", mb_game_tank) );
			checkboxes->add( new MiscButton("Waves", mb_game_waves) );
			checkboxes->add( new MiscButton("Floaters", mb_game_float) );
			checkboxes->add( new MiscButton("Colors", mb_game_color) );
			checkboxes->add( new MiscButton("Everything", mb_game_combined) );
			checkboxes->add( new ControlBase("Controls: Arrow keys, Space bar, WASD, F8, Esc") );
			
            checkboxes->commit();
            
		} break;			
	}
	
	want = wantAfterwards;
}

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	if (running) {
		program_clear();
		running = false;
		wantClearUi = true;
		want = WMainMenu;
		program_interface();
	} else {
		Quit(); // Default implementation just quits.
	}
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	// Default implementation does nothing.
}

bannerstream::~bannerstream() {
    displaying = str();
    displaying_life = life;
}