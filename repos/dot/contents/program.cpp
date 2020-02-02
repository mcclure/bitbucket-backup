// BUSINESS LOGIC

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andi McClure
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

#include "color.h"
#include "program.h"
#include "slice.h"
#include "internalfile.h"

#include <sys/stat.h>

// /------ Used by . ------\

chassis_thing chassis;

string onload_at = "0"; int onload_x = 64, onload_y = 64;

// \------ begin display state ------/

struct stitcher {
	slice *parent;
	int x,y,c; // X pos, Y pos, # consumed
	vector<stitch_square *> candidates;
	stitcher() : parent(NULL), x(-2),y(-2),c(0) {}
	void stitch(stitch_square *square, slice *s, int ax, int ay) {
		bool pushing = (ax == x || ay == y);
		if (!parent) { // "State 0"
			parent = s; x = ax; y = ay; pushing = true;
		} else if (s == parent && x >= 0 && y >= 0) { // "State 1"
			if (ax == x + 1) {
				x = -1; // Clear the channel that's increasing
			} else if (ay == y + 1) {
				y = -1;
			} else {
				pushing = false;
			}
		}
		if (pushing) { // "State 3"
			candidates.push_back(square);
		} else { // "State 4"
			if (c < candidates.size()) {
				stitch_square *partner = candidates[c];
				square->partner = partner;
				partner->partner = square;
				c++;
			} else {
				ERR("PARTNERLESS STITCH (1)\n");
			}
		}
	}
};

hash_map<unsigned int, stitcher *> stitch;

void sew_patch() {
    for(hash_map<unsigned int, stitcher *>::iterator b = stitch.begin(); b != stitch.end(); b++) {
		stitcher *color = (*b).second;
		for(int c = 0; c < color->candidates.size(); c++) {
			stitch_square *square = color->candidates[c];
			if (!square->partner) {
				ERR("PARTNERLESS STITCH (2)\n");
				continue;
			}
			for(int d = 0; d < 4; d++) {
				swap(square->nesw[d], square->partner->nesw[d]);
			}
		}
		delete color;
    }
    stitch.clear();
}

hash_map<string,const char *> scanon;
const char *canon(const string &s) {
	if (scanon.count(s)) {
		return scanon[s];
	} else {
		const char *ns = strdup(s.c_str());
		scanon[s] = ns;
		return ns;
	}
}

void load_patch(slice &s) {
	square_id *sid = NULL;
#if NAMETRACK
	const char *id = canon(s.name);
#endif
	int cx = -1, cy = -1;
	if (onload_at == s.name) {
		cx = onload_x; cy = onload_y;
	}
	
    square *building[s.width][s.height];
    for(int x = 0; x < s.width; x++) {
        for(int y = 0; y < s.height; y++) {
#if NAMETRACK
			square_id _sid(id,x,y);
			sid = &_sid;
#endif
            uint32_t &pixel = s.pixel[x][y];
            if (pixel == 0x000000FF) {          // Blank square
                building[x][y] = new square(sid);
            } else if (0xFF != (pixel & 0xFF)) {  // Warp square
                stitch_square *sq = new stitch_square(sid);
                building[x][y] = sq;
				stitcher *& hash = stitch[pixel];
                if (!hash)
					hash = new stitcher();
				hash->stitch(sq, &s, x, y);
            } else if (pixel == 0xFFFF00FF) {   // Bystander
                building[x][y] = new square(sid);
                (new bystander_thing(ntohl(pixel)))->attach(*building[x][y]);
                building[x][y]->blocked = true;
            } else {                                    // Wall
                building[x][y] = new plain_square( sid, ntohl(pixel), true );
            }
            
            if (x>0) { 
                building[x-1][y]->nesw[1] = building[x][y];
                building[x][y]->nesw[3] = building[x-1][y];
            }
            if (y>0) { 
                building[x][y-1]->nesw[2] = building[x][y];
                building[x][y]->nesw[0] = building[x][y-1];
            }
            
            if (x == cx && y == cy) {
                chassis.attach(*building[x][y]);
                building[x][y]->blocked = true;
            }
        }
    }
}

void load_patch(const char *named) {
    slice s;
    s.consume(named);
    load_patch(s);
}

void load_patch_named(string name) {
	char filename[FILENAMESIZE];
#if SELF_EDIT
	snprintf(filename, FILENAMESIZE, "../../Internal/%s", name.c_str());
	watch_file(filename);
#else
	internalPath(filename, name.c_str());
#endif
	load_patch(filename);
}


void thing::attach(square &to) {
    if (anchored)
        anchored->anchor.remove(this);
    anchored = &to;
    anchored->anchor.push_back(this);
}

bool dot_thing::can(int d) {
    square *n = anchored->nesw[d];
    return n && !n->solid && !n->blocked;
}

inline int bz(int z) {
    return z ? z : 1;
}

void dot_thing::reanchor() {
    anchored->blocked = false;
    attach(*anchored->nesw[md]);
    if (anchored->nesw[un(md)])
        anchored->nesw[un(md)]->blocked = true;
    offset.x = xe(offset.x, md, -1.0f);
    offset.y = ye(offset.y, md, -1.0f);    
}

void dot_thing::act() {
    if (mv) {
        mt++;
        if (mt == bz(mv/2)) {
            reanchor();
        }
        if (mt == mv) {
            mv = 0;
            offset = cpvzero;
            if (anchored->nesw[un(md)])
                anchored->nesw[un(md)]->blocked = false;
//            else ERR("WTF\n");
        } else {
            offset.x = xe(offset.x, md, 1.0f/mv);
            offset.y = ye(offset.y, md, 1.0f/mv);
        }
//        ERR("%d, %d, %d ; %f, %f\n", md, mt, mv, offset.x, offset.y);
    }
}

int cdir = -1;
bool hbc = false; // "have backup chassis" info
int bmd, bmv, bmt; // Backup motion direction, backup motion "velocity", backup motion tick

#define CHASSIS_SPEED 5

void chassis_thing::act() {
    if (!mv && cdir >= 0 && can(cdir)) {
        md = cdir;
        mv = CHASSIS_SPEED;
        mt = 0;
        anchored->nesw[md]->blocked = true;
    }
    dot_thing::act();
}

#define BYSTANDER_MOVE_CHANCE 50

void bystander_thing::act() {
    if (!mv && 0 == random() % BYSTANDER_MOVE_CHANCE) {
        int bdir = random() % 4;
        if (can(bdir)) {
            md = bdir;
            mv = CHASSIS_SPEED;
            mt = 0;
            anchored->nesw[md]->blocked = true;
        }
    }
    dot_thing::act();
}

// check_updates mechanism

#if SELF_EDIT
struct watched_file {
	timespec updated;
	string name;
	watched_file(const string &_name = string()) : name(_name) { 
		memset( &updated, 0, sizeof(updated) );
	}
};

vector<watched_file> watching_files;

int last_check_updates = 0;
#endif

void watch_file(const string &filename) {
#if SELF_EDIT
	watching_files.push_back(watched_file(filename));
#endif
}

void check_updates() {
#if SELF_EDIT
	int now = SDL_GetTicks();
	if (now - last_check_updates < 1000)
		return;
	last_check_updates = now;
	
	bool changes = false;
	
	for(int c = 0; c < watching_files.size(); c++) {
		timespec old_updated = watching_files[c].updated;
		struct stat stats;
		
		stat(watching_files[c].name.c_str(), &stats);
		
		watching_files[c].updated = stats.st_mtimespec;
				
		if (old_updated.tv_sec && (old_updated.tv_sec != watching_files[c].updated.tv_sec
			|| old_updated.tv_nsec != watching_files[c].updated.tv_nsec)) {
			ERR("CHANGED: %s\n", watching_files[c].name.c_str());
			changes = true;
		}
	}
	
	if (changes)
		program_reset();
#endif
}

// This is called once when the program begins. Clear out and replace with your own code
void
program_init(void)
{
    load_patch_named("0.png");
	load_patch_named("mos.png");

    sew_patch();
}

typedef hash_map<void *, void *> ptr_set;

// Armaggeddon
void wipe(square *at, ptr_set *visited) {
	(*visited)[at] = at;
	
	for(int c = 0; c < 4; c++) {
		square *next = at->nesw[c];
		if (next && !visited->count(next)) {
			wipe(next, visited);
		}
	}
	
    for(list<thing *>::iterator b = at->anchor.begin(); b != at->anchor.end(); b++) {
		if (*b != &chassis)
			delete *b;
	}
	
	delete at;
}

void program_reset() {
	const square_id &id = chassis.anchored->id;	
	onload_at = id.id;
	onload_x = id.x;
	onload_y = id.y;
	
	{
		ptr_set visited;
		wipe(chassis.anchored, &visited);
		chassis.anchored = NULL;
	}
	
#if SELF_EDIT
	watching_files.clear();
#endif
	
	program_init();
}

// The default display() calls this once per framedraw.
void
program_update(void)
{
	if (paused) return; // No motion when paused
	
    for(vector<thing *>::iterator b = acting.begin(); b != acting.end(); b++)
        (*b)->act();
	
	check_updates();
}

int count_down = 0;
int nesw_down[4];

// This is called when SDL gets a key event. Clear out and replace with your own code
void program_eventkey(SDL_Event &event) {
    SDLKey &key = event.key.keysym.sym;

    int ndir = -2;
    
	if (event.type == SDL_KEYDOWN) { 
        int pdir = -1;
        
		if (key == SDLK_F1) {
            optWindow = !optWindow;
            recreate_surface(optWindow);
		} else if (key == SDLK_UP) {
            pdir = 0;
        } else if (key == SDLK_DOWN) {
            pdir = 2;
        } else if (key == SDLK_LEFT) {
            pdir = 3;
        } else if (key == SDLK_RIGHT) {
            pdir = 1;
        }
        
        if (pdir >= 0 && count_down < 4) {
            ndir = pdir;
            nesw_down[count_down++] = pdir;
            if (1 == count_down && hbc) {
                chassis.md = bmd;
                chassis.mv = bmv;
                chassis.mt = bmt;
                hbc = false;
            }
        }
		
#if NAMETRACK
		if (key == '`') {
			program_reset();
		}
#endif
    } else if (event.type == SDL_KEYUP) {
        int udir = -1;
        
        if (key == SDLK_UP) {
            udir = 0;
        } else if (key == SDLK_DOWN) {
            udir = 2;
        } else if (key == SDLK_LEFT) {
            udir = 3;
        } else if (key == SDLK_RIGHT) {
            udir = 1;
        }        
        
        if (udir >= 0) {
            int down = 0;
            while(down<count_down) {
                if (nesw_down[down] == udir) {
                    for(int e = down; e < count_down-1; e++)
                        nesw_down[e] = nesw_down[e+1];
                    count_down--;
                } else {
                    down++;
                }
            }
            
            if (count_down == 0) {
                if (chassis.mv && chassis.mt) {
                    hbc = true;
                    bmd = chassis.md;
                    bmv = chassis.mv;
                    bmt = chassis.mt;
                    chassis.mv = 0;
                }
                ndir = -1;
            } else {
                ndir = nesw_down[count_down-1];
            }
        }
    }
    
    if (ndir >= -1) {
        if (ndir >= 0 && ndir == un(chassis.md)) {
            chassis.mt = chassis.mv - chassis.mt - 1;
            chassis.md = ndir;
            
            // UNACCEPTABLE KLUDGE -- SMELLS LIKE TILT
            // WILL FIX SELF IF I MAKE MT A FLOAT AND TEST ON BOUNDARIES?
            if (chassis.mt == bz(chassis.mv/2)) {
                chassis.reanchor();
            }            
        }
        cdir = ndir;
    }
}

// This is called when SDL gets a joystick event.
void program_eventjoy(SDL_Event &event) {
}

// This is called when SDL gets a mouse event.
void program_eventmouse(SDL_Event &event) {
}

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	Quit(); // Default implementation just quits.
}

// Called after SDL quits, right before program terminates. Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
}