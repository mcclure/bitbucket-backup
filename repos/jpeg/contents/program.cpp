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
#include <dirent.h>

#include "chipmunk.h"
#include "color.h"
#include "program.h"
#include "tinyxml.h"
#include "internalfile.h"

#define GRAVITY cpv(0.0f, -9.8/2)

vector<spaceinfo> spaces;

bool wantClearUi = false;
WantNewScreen want = WNothing, onEsc = WNothing;
cpSpace *uiSpace = NULL;
vector<ContainerBase *> columns;

list<automaton *> automata;
vector<auto_iter> dying_auto;

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

bannerstream::~bannerstream() {
    displaying = str();
    displaying_life = life;
}

// /------ State used by the "ABCD" demo ------\

extern hash_map<string, texture_atlas *> atlas_root;
extern bool toggle_fire;

enum LBKind {
	LBLoadFile,
};

class LoaderButton : public ControlBase { 
public:
	LoaderButton(string _text, LBKind) : ControlBase(_text, true) {}
	
	virtual void click() {
		machine = new imagemachine(text);
		onEsc = WMainMenu;
		wantClearUi = true;
	}
};

// \------ begin actual code ------/

void appendLevel(double depth) {
	int s = spaces.size(); // Is any of this even used?
  /* We first create a new space */
  spaces.push_back(spaceinfo());
  spaces[s].depth = depth;  
  
	spaces[s].space = cpSpaceNew();
	
	/* Next, you'll want to set the properties of the space such as the
     number of iterations to use in the constraint solver, the amount
     of gravity, or the amount of damping. In this case, we'll just
     set the gravity. */
	spaces[s].space->gravity = GRAVITY;
	
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
program_init(bool reinit)
{
	if (!reinit) { // First boot
		cpInitChipmunk();
		uiSpace = cpSpaceNew();		
		want = WMainMenu;
	} else { // Resets after first boot
		// Note: This path is never reached in the demo. If it were, though, it would be a memory leak.
		// You should be actually destroying the spaces and objects whenever you clear the spaces list.
		spaces.clear();
	}
	
	appendLevel(0);
    
    cpVect screen_verts[] = {
		cpv(-1/aspect,-1),
		cpv(-1/aspect, 1),
		cpv( 1/aspect, 1),
		cpv( 1/aspect,-1),
	};        
	
	for(int c = 0; c < 4; c++) {		
		cpShape *newShape = cpSegmentShapeNew(spaces[0].staticBody, screen_verts[c], screen_verts[(c+1)%4], 0.1);
		cpSpaceAddStaticShape(spaces[0].space, newShape);
		newShape->e = 0.8; newShape->u = 0.8; newShape->collision_type = C_WALL;
	}	
}

// The default display() calls this once per framedraw.
void
program_update(void)
{
    ticks++;

	/* Collision detection isn't amazing in Chipmunk yet. Fast moving
     objects can pass right though each other if they move too much in a
     single step. To deal with this, you can just make your steps
     smaller and cpSpaceStep() several times. */
  int substeps = 1;

  /* This is the actual time step that we will use. */
  // Note: Chipmunk will not perform as well unless the step stays fixed over time
#if !DYNAMIC_FRAMERATE
  cpFloat dt = (1.0f/FPS) / (cpFloat)substeps;
#else
  cpFloat dt = sinceLastFrame / 1000.0 / (cpFloat)substeps;
#endif
    
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
        
#if 0 // No chipmunk step
    int i;
  for(i=0; i<substeps; i++){      
	  for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
		spaceinfo *s = &(*_s);
		if (okayToDraw(s)) {		
			// Iterate space normally
			cpSpaceStep(s->space, dt);
		}
	  }
	}
#endif
	
	check_updates(); // See internalFile.h
	
	// jpeg
	
	if (machine && machine->f) {
		int total = machine->pending + machine->ongoing;
		bool reset = false;
		if (total > 0) {
			reset = true;
			patch apply(
						random() % machine->f->data.size(),
						1 << (random() % 8));
			machine->f->data[apply.at] ^= apply.mask;
			machine->f->patches.push_back(apply);
		} else if (total < 0) {
			total = min<int>(machine->f->patches.size(), -total);
			while (total > 0) {
				const patch &back = machine->f->patches.back();
				machine->f->data[back.at] ^= back.mask;
				machine->f->patches.pop_back();
				total--;
				reset = true;
			}
		}
		machine->pending = 0;
		
		if (reset)
			machine->resetTexture();
	}
}

// A funnel suitable for handling either mouse or touchscreen events
void program_metamouse(dragtouch &d, touch_type t, cpVect at) {
	// I suggest keeping this little 4-line sigil at the top so that the interface library will work:
	if (t == touch_down && interface_attempt_click(at, d.button)) // Let the interface library look at the event first
		d.special = dragtouch::interface_special;
	if (d.special == dragtouch::interface_special) // Once a touch has been marked as part of the interface library, ignore it
		return;
	
	// Put your touch/click handler code here
}

#ifdef TARGET_SDL

void program_eventkey(SDL_Event &event) {
    SDLKey &key = event.key.keysym.sym;

	if (!machine) return;
	
	// Put your keyboard handler code here
	if (event.type == SDL_KEYDOWN) { // DEBUG KEYS
        if (key == SDLK_LEFT) { machine->pending--; }
        if (key == SDLK_RIGHT) { machine->pending++; }
	}
}

// This is called when SDL gets a joystick event.
void program_eventjoy(SDL_Event &event) {
}

// TODO: It would be interesting to introduce separate drag objects for left/right click.
dragtouch globaldrag;

void program_eventmouse(SDL_Event &event) { // Probably you want to leave this as it is and use metamouse
    touch_type t = touch_move;
    if (event.type == SDL_MOUSEBUTTONDOWN)
        t = touch_down;
    if (event.type == SDL_MOUSEBUTTONUP)
        t = touch_up;
	
	globaldrag.button = event.button.button;
    program_metamouse(globaldrag, t, cpv(event.button.x, event.button.y));
	globaldrag.special = dragtouch::not_special; // Don't "poison" the main mouse event
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
	int nSpaces = spaces.size();
	int s;
	float mag = GRAVITY.y;
	for (s=0;s<nSpaces;s++) {
		spaces[s].space->gravity = cpv(-x*mag, -y*mag);
	}
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
			if (machine) {
				delete machine;
				machine = NULL;
			}
			
			ContainerBase *info = new ContainerBase(uiSpace, CMIDL);
			columns.push_back(info);
			info->add( new ControlBase("jpegfuck") );
			info->commit();
			
			ContainerBase *listing = new ScrollContainer(uiSpace, CRIGHT);
			columns.push_back(listing);
			
			DIR *dird = opendir(".");
			dirent *dir;
			
			const char *supported[] = {".jpg", ".jpeg", ".png", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic", NULL}; 
			
			while (dir = readdir(dird)) {
				bool valid = false;
				int namlen = strlen(dir->d_name);
				int extlen = 0;
				for(int c = 0; supported[c] && !valid; c++) {
					extlen = strlen(supported[c]);
					if (namlen >= extlen && !strncmp( supported[c], dir->d_name + namlen - extlen, extlen ))
						valid = true;
				}
				if (valid) {
//					dir->d_name[namlen - extlen] = '\0'; // THIS DOESN'T HAVE SIDE EFFECTS I ASSUME
					listing->add( new LoaderButton(dir->d_name, LBLoadFile) );
				}
			}
			listing->commit();
			onEsc = WNothing;
            
            break;
	}
	
	want = wantAfterwards;
}

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	if (onEsc) {
		want = onEsc;
		wantClearUi = true;
		program_interface();
	} else {
		Quit(); // Default implementation just quits.
	}
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	// Default implementation does nothing.
}

// jpegfuck

imagemachine *machine = NULL;

imagemachine::imagemachine(const string &_name) : name(_name), f(NULL), rawtexture(NULL), subtexture(NULL), pending(0), ongoing(0), displayInfo(true) {
	f = new imagefile();
	
	FILE *file = fopen(name.c_str(), "r");
	if (file) {
		long length = 0; fseek( file, 0, SEEK_END ); length = ftell( file );
		fseek( file, 0, SEEK_SET );
		f->data.resize(length);
		fread(&f->data[0], sizeof(unsigned char), length, file);
		fclose(file);
		
		resetTexture();
	}
}

void imagemachine::resetTexture() {
	delete rawtexture;
	delete subtexture;
	
	rawtexture = new texture_slice();
	rawtexture->consume(name.c_str(), &f->data[0], f->data.size());
	rawtexture->construct();
	subtexture = rawtexture->sub();
}

imagemachine::~imagemachine() {
	delete f;
	delete rawtexture;
	delete subtexture;
}