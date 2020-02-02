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
#include <list>
#include <vector>
#include <map>
#include <string>

#include "chipmunk.h"
#include "color.h"
#include "program.h"
#include "tinyxml.h"
#include "internalfile.h"
#include "input.h"
#include "game.h"

bool wantClearUi = false;
WantNewScreen want = WNothing;
cpSpace *uiSpace = NULL;
vector<ContainerBase *> columns;

bannerstream::~bannerstream() {
    displaying = str();
    displaying_life = life;
}

// /------ State used by the "ABCD" demo ------\

extern hash_map<string, texture_atlas *> atlas_root;

// \------ begin actual code ------/


// This is called once when the program begins. Clear out and replace with your own code
void
program_init(bool reinit)
{
	if (!reinit) { // First boot
		cpInitChipmunk();
		uiSpace = cpSpaceNew();		
	}
	
	game_init();
}

// The default display() calls this once per framedraw.
void
program_update(void)
{
    ticks++;
    
	ent::global_tick();
	
	check_updates(); // See internalFile.h
}

// A funnel suitable for handling either mouse or touchscreen events
void program_metamouse(touch_rec &rec, touch_type t) {
	// I suggest keeping this little 4-line sigil at the top so that the interface library will work:
	
	// Put your touch/click handler code here
	InputData data;
	data.kind = InputKindTouch;
	data.touchkind = t;
	data.touch = rec;
		
	InputRules::rules()->route(&data);
}

#ifdef TARGET_SDL

// TODO: Route should be able to split HIGH/LOW
void program_eventkey(SDL_Event &event) {
    SDL_KeyboardEvent &key = event.key; //.keysym.sym

	InputData data;
	data.kind = InputKindKeyboard;
	data.key = key;
		
	InputRules::rules()->route(&data);	
}

// This is called when SDL gets a joystick event.
void program_eventjoy(SDL_Event &event) {
	switch(event.type) {
		case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP: {
			InputData data;
			data.kind = InputKindButton;
			data.jbutton = event.jbutton;
			
			InputRules::rules()->route(&data);
		} break;
		case SDL_JOYHATMOTION: {
			InputData data;
			data.kind = InputKindHat;
			data.jhat = event.jhat;
			
			InputRules::rules()->route(&data);
		} break;
		case SDL_JOYAXISMOTION: {
			InputData data;
			data.kind = InputKindAxis;
			data.jaxis = event.jaxis;
			
			InputRules::rules()->route(&data);
		} break;
	}
}

void program_eventrecontroller(SDL_Event &event) {
	InputData data;
	data.kind = InputKindSystem;
	data.systemkind = InputSystemRecontroller;
	
	InputRules::rules()->route(&data);
}

// TODO: It would be interesting to introduce separate drag objects for left/right click.
touch_rec globaldrag;

void program_eventmouse(SDL_Event &event) { // Probably you want to leave this as it is and use metamouse
    touch_type t = touch_move;
    if (event.type == SDL_MOUSEBUTTONDOWN)
        t = touch_down;
    if (event.type == SDL_MOUSEBUTTONUP)
        t = touch_up;
	
	globaldrag.button = event.button.button;
	globaldrag.at = cpv(event.button.x, event.button.y);
    program_metamouse(globaldrag, t);
	globaldrag.special = touch_rec::not_special; // Don't "poison" the main mouse event
}
#endif

#ifdef TARGET_MOBILE

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
                allTouches.erase(i->tid);
            } break;
        }
    }
}

void program_eventaccel(const float &x, const float &y, const float &z) {
}

void program_sleep() {
	ERR("Going to sleep\n");
}

void program_wake() {
	ERR("Waking up\n");
}

#endif

// The "this is a good time to modify your uiSpace" function. Main.cpp calls this once at the beginning of the main
// loop and once every time a button is successfully clicked. This exists because it's unsafe to modify the interface
// from inside of a ControlBase click() handler, so instead click() handlers can stash some state that this function
// then picks up when main.cpp automatically calls it after the click is completed...
void program_interface() {
	WantNewScreen wantAfterwards = WNothing; // "want" will be cleared with this at the end of the function
	
	if (wantClearUi) {
		for(int c = 0; c < columns.size(); c++)
			if (columns[c])
				delete columns[c];
		
		columns.clear();
		
		resetScrollmax();
		wantClearUi = false;
	}
	
	switch (want) {	
        case WMainMenu: // Clear this out and replace it with whatever you want:
#if 0
			ContainerBase *abcd = new ContainerBase(uiSpace, CMIDL);
			columns.push_back(abcd);
			
            abcd->add( new SpawnerButton("A" , atlas_root["interface-1.png"]->images["a"]) );
			abcd->add( new SpawnerButton("B" , atlas_root["interface-1.png"]->images["b"]) );
			abcd->add( new SpawnerButton("C" , atlas_root["interface-1.png"]->images["c"]) );
			abcd->add( new SpawnerButton("D" , atlas_root["interface-1.png"]->images["d"]) );
            
            abcd->commit();
#endif
            break;
	}
	
	want = wantAfterwards;
}

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	Quit(); // Default implementation just quits.
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	InputData data;
	data.kind = InputKindSystem;
	data.systemkind = InputSystemQuit;
	
	InputRules::rules()->route(&data);
}	