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

// defaults is where the actual controls live. file_defaults is a cache of the contents of the
// controls file. this is so that if we have to disregard instructions from the controls file
// (eg because a joystick was not found) we can avoid overwriting that slot.
player_controls file_defaults[MAX_PLAYERS], defaults[MAX_PLAYERS];
string file_jnames[MAX_PLAYERS];

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

// /------ State used by Hello ------\

// None yet.

// \------ begin actual code ------/

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	if (cli) // if there's a cli, it decides what to do with esc.
		cli->BackOut();
	else	 // if there's NO cli, then... WTF?
		Quit();
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	// Default implementation does nothing.
}

// This is called once when the program begins. Clear out and replace with your own code
void
program_init(void)
{
    cpInitChipmunk();
	uiSpace = cpSpaceNew();
	    	
	load_controls(); // Will possibly overwrite defaults
	
	ERR("END LEVEL LOAD\n");					 
	
    program_reinit();
}

void
program_reinit(void)
{
    newtype(new basic_automaton());
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
    
	op_interface::reset_cycles();
	
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