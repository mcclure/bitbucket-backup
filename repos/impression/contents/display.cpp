// DISPLAY CODE

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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "kludge.h"

#include "chipmunk.h"
#include "lodepng.h"
#include "internalfile.h"
#include <queue>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "tinyxml.h"
#include "program.h"
#include "display.h"
#include "color.h"

#include "glCommon.h"
#include "glCommonMatrix.h"

#define RAINBOW_DEBUG 0
#define SPLATTER 0
// #define FADE 0.5

// TODO: THIS IS WAY TOO MUCH STATE! PUT THIS IN AN OBJECT OR SOMETHING
#define FCIRCLE_SLICES 3
#define WALK_FRAMES 4
slice *s_stand, *s_jump, *s_walk[WALK_FRAMES];

hash_map<string, texture_atlas *> atlas_root;

extern double radmin; // TODO move to program.h

unsigned int randomColor() {
    return packColor((double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, 1);
}

inline void Color4f(double r, double g, double b, double a) { // Awful
#if RAINBOW_DEBUG
    static int last_rainbow_ticks = 0;
    static int this_rainbow_ticks = 0;
    static vector<unsigned int> _color;
    
    if (last_rainbow_ticks != ticks) {
        this_rainbow_ticks = 0;
        last_rainbow_ticks = ticks;
    } else {
        this_rainbow_ticks++;
    }
    if (this_rainbow_ticks >= _color.size()) {
        _color.push_back( randomColor() );
    }
    
	jcColor4ubv((GLubyte *)&_color[this_rainbow_ticks]);
#else
	jcColor4f(r, g, b, a);
#endif
}

// Protect with a #?
string displaying;
int displaying_life = 0;

RunMode run_mode = run_game;

float light_min_dist = 1;

// ---------------- "OBJECTS" ---------------------

// Automata

double randco() {
    return (double)random()/RANDOM_MAX*2-1;
}

struct wave_auto : automaton {
    wave_auto() : automaton() {}
    void display(blitter *b) {
        float *gridbase = &b->grid[0], *backbase = &b->backup[0]; // Grid used twice
        for(int y = 0; y < b->h; y++) {
            for(int x = 0; x < b->w; x++) {
                int c = b->c(x,y);
                cpVect *backup = (cpVect *)(backbase+c*b->stride), // b->stride arithmetic occurring too late
                    *grid = (cpVect *)(gridbase+c*b->stride);
                float theta = x+y+(ticks-born)/30.0;
                *grid = cpvadd(*backup, cpvmult(cpv(sin(theta),sin(theta)), b->free_space));
            }
        }
    }
};

struct click_auto : automaton {
    int offset;
    click_auto(int _offset = 0) : automaton(), offset(_offset) {
        rollover = FPS/2;
    }
    void tick() {
        automaton::tick();
        switch(state) {
            case 1: {
                die();
            } break;
        }        
    }
    void display(blitter *b) {
        float at = float(frame)/rollover; at = 1-at; at *= at; at = 1-at;
		if (0 == frame) {
			extern int tapping; tapping = 0; // DON'T DO IT LIKE THIS
		}
        for(int y = 0; y < b->h; y++) {
            for(int x = 0; x < b->w; x++) {
                int c = b->c(x,y);
                float &backup = b->backup[c*b->stride+offset], // b->stride arithmetic occurring too late
                    &grid = b->grid[c*b->stride+offset];
                float plan_o = backup-b->free_space + at*b->free_space*2;
                grid = grid < plan_o ? grid : plan_o;
            }
        }
    }
};

// Dot box rendering

// Not for actual game use really
#define TEST_BOXSIZE 5
struct test_square_dotbox : public dotbox {
    test_box_environ *e;
    test_square_dotbox(int _w = 0, int _h = 0, test_box_environ *_e=NULL) : dotbox(_w,_h), e(_e) {}
    virtual void update() {
        if (ticks<updated) return; updated = ticks;
        
        for(int y = 0; y < h; y++) {
            for(int x = 0; x < w; x++) {
                grid[c(x,y)] =
                    (iabs(x-e->x) < TEST_BOXSIZE && iabs(y-e->y) < TEST_BOXSIZE) ? 0:1;
            }
        }
    }
};

// In progress
struct level_dotbox : public dotbox {
    level_environment *e;
    level_dotbox(int _w = 0, int _h = 0, level_environment *_e=NULL) : dotbox(_w,_h), e(_e) {}
    virtual void update() {
        if (ticks<updated) return; updated = ticks;
        
        for(int _y = 0; _y < h; _y++) {
            for(int _x = 0; _x < w; _x++) {
                int x = _x+e->x, y = _y+e->y;
                int idx = c(_x,_y);
                if (!e->l->inside(x,y)) {
                    grid[idx] = -1;
                } else {
                    unsigned int pixel = e->l->pixel[x][y];
                    if (pixel == EC_WATER || pixel == EC_DEEPWATER || pixel == EC_PLAYER) {
                        float theta = x+y+(ticks)/30.0;
                        grid[idx] = (sin(theta)+1)/2;
                    } else if (pixel == EC_FLAME) {
                        float theta = (double)random()/RANDOM_MAX;
                        grid[idx] = theta;
                    } else {
                        grid[idx] = (pixel&0xFF)/255.0;
                    }
                }
            }
        }

#if 0
        struct drawme : eobject::callback { level_environment *e; virtual bool check(int x, int y) {
            return !e->l->inside() || (
        } }; drawme l = {e};
#endif
        for(eobject_iter i = e->objects.begin(); i != e->objects.end(); i++) {
            slice *paste = NULL;
            bool xflip = (*i)->xflip;
            
            switch ((*i)->ec) {
                case EC_PLAYER: {
                    player_eo *p = (player_eo *)(*i);
					if (p->dead) {
						break;
					} else if (!p->grounded) {
						paste = s_jump;
					} else if (p->run_start < 0) {
                        paste = s_stand;
                    } else {
                        paste = s_walk[((ticks-p->run_start)/4)%4];
                    }
                } break;
            }
            
            if (paste) {
                for(int _y = 0; _y < paste->height; _y++) {
                    for(int _x = 0; _x < paste->width; _x++) {
                        int x = (*i)->x1 + _x - e->x;
                        int y = (*i)->y1 - _y - e->y;
                        if (x < 0 || y < 0 || x >= w || y >= h) continue;
                        int alpha = paste->pixel[xflip?paste->width-_x-1:_x][_y]&0xFF;
                        if (alpha)
                            grid[c(x,y)] = alpha/255.0;
                    }
                }
            }
        }
    }
};

// Channels

// This bit of the code is gonna get cluttered quick if I'm not careful...

// I think this is, in fact, completely useless. Can I just delete it?
struct copy_channel : public channel {
    int offset;
    copy_channel(dotbox *_from = NULL, int _offset = 0) : channel(_from), offset(_offset) {}
    virtual void post(blitter *b) {
        from->update();
        for(int y = 0; y < b->h; y++) {
            for(int x = 0; x < b->w; x++) {
                float i = from->grid[from->c(x,y)];
                float &o = b->grid[ b->c(x,y)*b->stride + offset ];
                o = i;
            }
        }
    }
};

struct grayscale_channel : public channel {
    grayscale_channel(dotbox *_from = NULL) : channel(_from) {}
    grayscale_channel(dotbox *_from, float, float) : channel(_from) {} // So can drop in for dampen_channel
    virtual void post(blitter *b) {
        from->update();
        for(int y = 0; y < b->h; y++) {
            for(int x = 0; x < b->w; x++) {
                float i = from->grid[from->c(x,y)];
                float *of = &b->grid[ b->c(x,y)*b->stride + b->color_off ];
                unsigned int &o = *(unsigned int *)of;
                if (i >= 0) {
                    o = packColor(i,i,i,1);
                } else {
                    o = packColor(1,0,0,1);
                }
            }
        }
    }
};

// Some pretty egegrious code duplication here
struct color_channel : public channel {
   color_channel(dotbox *_from = NULL) : channel(_from) {}
   color_channel(dotbox *_from, float, float) : channel(_from) {} // So can drop in for dampen_channel
   virtual void post(blitter *b) {
	   from->update();
	   for(int y = 0; y < b->h; y++) {
		   for(int x = 0; x < b->w; x++) {
			   float i = from->grid[from->c(x,y)];
			   float *of = &b->grid[ b->c(x,y)*b->stride + b->color_off ];
			   unsigned int &o = *(unsigned int *)of;
			   if (i >= 0) {
				   o = packColor(1-i,i,i,1);
			   } else {
				   o = packColor(0,1,0,1);
			   }
		   }
	   }
   }
};									   
									   
struct position_channel : public channel {
    int offset;
    position_channel(dotbox *_from = NULL, int _offset = 0) : channel(_from), offset(_offset) {}
    virtual void post(blitter *b) {
        from->update();
        for(int y = 0; y < b->h; y++) {
            for(int x = 0; x < b->w; x++) {
                int idx = b->c(x,y)*b->stride + offset;
                float &i = from->grid[from->c(x,y)];
                float &o = b->grid[ idx ];
                float &backup = b->backup[ idx ];
                o = backup+(i*2-1)*b->free_space;
            }
        }
    }
};

// Copy of position_channel which has a delta limit
struct dampen_channel : public channel {
    int offset;
    float framemax;
    dampen_channel(dotbox *_from = NULL, int _offset = 0, float _framemax = 1) : channel(_from), offset(_offset), framemax(_framemax) {}
    virtual void post(blitter *b) {
        from->update();
        for(int y = 0; y < b->h; y++) {
            for(int x = 0; x < b->w; x++) {
                int idx = b->c(x,y)*b->stride + offset;
                float &i = from->grid[from->c(x,y)];
                float &o = b->grid[ idx ];
                float &backup = b->backup[ idx ];
                float plan_o = backup+(i*2-1)*b->free_space;
                
                if (plan_o < o-framemax) o = o-framemax;
                else if (plan_o > o+framemax) o = o+framemax;
                else o = plan_o;
            }
        }
    }
};

// Blitter

blitter *blit = NULL;

void blitter::init(int _w, int _h, float _spacing, float _dotw, float _dotvary) {
    w = _w; h = _h; spacing = _spacing; dotw = _dotw; dotvary = _dotvary;
    free_space = spacing/2 * 0.5;
    
    base = cpv(-spacing*(w-1)/2, -spacing*(h-1)/2);
    
    stride = VERT_STRIDE+COLOR_STRIDE+SIZE_STRIDE;
    point_off = 0; x_off = 0; y_off = 1;
    color_off = VERT_STRIDE;
    size_off = VERT_STRIDE+COLOR_STRIDE;
    
    grid.clear(); // So we can call this more than once?
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            backup.push( cpv( base.x + spacing*x, base.y + spacing*y ), true, 1,1,1,1,dotw);
            grid.push( cpv( base.x + spacing*x, base.y + spacing*y ), true, 1,1,1,1,dotw);
        }
    }
}

void blitter::display() {
    for(channel_iter i = channels.begin(); i != channels.end(); i++)
        (*i)->post(this);
    
    for(auto_iter i = automata.begin(); i != automata.end(); i++)
        (*i)->display(this);
    
    States(false, true);// glEnableClientState(GL_POINT_SIZE_ARRAY);
    jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &grid[0]+point_off);
    jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &grid[0]+color_off);
    //glPointSizePointer(1, GL_FLOAT, stride*sizeof(float), &grid[0]+size_off);
    glDrawArrays(GL_POINTS, 0, w*h);            
}

// ------------- OPENGL WRAPPERS ------------------

// This is called once each time the display surface is initialized. It's a good place to do things like initialize
// fonts and textures. (Note it could be called more than once if the window size ever changes.)
void display_init(bool reinit) { // TODO: Use reinit
    // Boilerplate
	rinseGl();
    jcMatrixInit();
    
    ERR("Screen size %fx%f", (float)surfacew, (float)surfaceh);
    
    text_init();
    
    // If next line is uncommented, atlas copying must be added back to windows makefile
//    atlas_root = atlas_load("atlasDictionary.xml"); // Why bother
    
    // Initialize objects
	if (!reinit) {
		s_stand = new slice();   s_stand->iload("stand.png");
		s_jump = new slice();    s_jump->iload("jump.png");
		s_walk[0] = new slice(); s_walk[0]->iload("walk1.png");
		s_walk[1] = new slice(); s_walk[1]->iload("walk2.png");
		s_walk[2] = new slice(); s_walk[2]->iload("walk3.png");
		s_walk[3] = new slice(); s_walk[3]->iload("walk4.png");
	}
    
    // Actually do stuff
#if 0
#if 1
    glEnable(GL_POINT_SPRITE);
	glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
#else
    glEnable(GL_POINT_SPRITE_OES);
    glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
#endif
#endif
    
	glDisable(GL_DEPTH_TEST);
	glPointSize(10.0); // TODO: Scale
    glEnable(GL_POINT_SMOOTH); // ??
    
    glClearColor(0,0,0,1);
}

void MiscButton::click() {
	wantClearUi = true;
	running = true;
	int w = 30, h = 20;
	
	switch (t) {		
	   case mb_game_waves: {		   
		   // DISPLAY SETUP -- NOTHING ELSE
		   blit = new blitter(); blit->init(w, h, 0.09, 5, 2);
		   		   
		   (new wave_auto())->push();		   
	   } break;
		   
	   case mb_game_tank: {
		   // PROGRAM SETUP
		   base_environment = new test_box_environ(w/2,h/2);
		   base_environment->push();
		   
		   // DISPLAY SETUP
		   blit = new blitter(); blit->init(w, h, 0.09, 5, 2);
		   
		   dotbox *basic_dotbox = new test_square_dotbox(w, h, (test_box_environ *)base_environment); // Someday this will be too simplistic.
		   
		   blit->channels.push_back( new dampen_channel(basic_dotbox, blit->y_off, 0.01) );
		   
		   (new click_auto(blit->y_off))->push();
	   } break;
		   
	   case mb_game_float: {
		   // PROGRAM SETUP
		   slice *level1 = new slice(); level1->iload("level1.png", false, true);
		   base_environment = new level_environment(0,0, level1);
		   base_environment->push();
		   ((level_environment *)base_environment)->init(); // Ugh	
		   
		   // DISPLAY SETUP
		   blit = new blitter(); blit->init(w, h, 0.09, 5, 2);
		   
		   dotbox *basic_dotbox = new level_dotbox(w, h, (level_environment *)base_environment); // Someday this will be too simplistic.
		   
		   blit->channels.push_back( new dampen_channel(basic_dotbox, blit->y_off, 0.01) );
		   
		   (new click_auto(blit->y_off))->push();
	   } break;
		   
	   case mb_game_color: {		   
		   // PROGRAM SETUP
		   slice *level1 = new slice(); level1->iload("level1.png", false, true);
		   base_environment = new level_environment(0,0, level1);
		   base_environment->push();
		   ((level_environment *)base_environment)->init(); // Ugh	
		   
		   // DISPLAY SETUP
		   blit = new blitter(); blit->init(w, h, 0.09, 5, 2);
		   
		   dotbox *basic_dotbox = new level_dotbox(w, h, (level_environment *)base_environment); // Someday this will be too simplistic.
		   
		   blit->channels.push_back( new grayscale_channel(basic_dotbox, blit->y_off, 0.01) );
		   
		   SDLMod mods = SDL_GetModState();
		   if (mods & KMOD_LCTRL || mods & KMOD_RCTRL) {
			   (new wave_auto())->push();		   
		   }
	   } break;
		   
	   case mb_game_combined: {
		   // PROGRAM SETUP
		   slice *level1 = new slice(); level1->iload("level1.png", false, true);
		   base_environment = new level_environment(0,0, level1); // 4 is a magic number!!
		   base_environment->push();
		   ((level_environment *)base_environment)->init(); // Ugh	
		   
		   ((level_environment *)base_environment)->arrow_trigger[0] = (SDLKey)'w';
		   ((level_environment *)base_environment)->arrow_trigger[1] = (SDLKey)'s';
		   ((level_environment *)base_environment)->arrow_trigger[2] = (SDLKey)'d';
		   ((level_environment *)base_environment)->arrow_trigger[3] = (SDLKey)'a';
		   
		   slice *level1flip = new slice(); level1flip->iload("level1.png", true, true);
		   level_environment *flip_environment = new level_environment(level1->width-w,0, level1flip); // 4 is a magic number!!
		   flip_environment->push();
		   flip_environment->init(); // Ugh!
		   
		   flip_environment->player->x1 -= 4;
		   
		   // DISPLAY SETUP
		   blit = new blitter(); blit->init(w, h, 0.09, 5, 2);
		   
		   dotbox *basic_dotbox = new level_dotbox(w, h, (level_environment *)base_environment); // Someday this will be too simplistic.
		   dotbox *flip_dotbox = new level_dotbox(w, h, (level_environment *)flip_environment); // Someday this will be too simplistic.		   

		   blit->channels.push_back( new color_channel(basic_dotbox, blit->y_off, 0.01) );		   
		   blit->channels.push_back( new dampen_channel(flip_dotbox, blit->y_off, 0.01) );		   
	   } break;
	}
}
									   
									   
#ifdef FADE
void dim() {
    quadpile scratch2;
    scratch2.push(-1/aspect,-1,1/aspect,1);
    scratch2.megaIndexEnsure(6);
    States(false, false); // Control boxes-- no fog   
    jcColor4f(0.0,0.0,0.0,FADE);
    jcVertexPointer(2, GL_FLOAT, VERT_STRIDE*sizeof(float), &scratch2[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);        
}
#endif

cpVect noturn = cpvforangle(0);
quadpile _scratch; int pushed = 0;
linepile _scratch_lines; int pushed_lines = 0;
pile _scratch_points; int pushed_points = 0;

// This is called when it is time to draw a new frame.
void display(void)
{	
    EnableClientState(GLCS_VERTEX); // TODO: What state to set where?

#ifdef FADE
    dim();
#endif
    
#if SPLATTER
    glClear(GL_DEPTH_BUFFER_BIT);
#else
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

	goOrtho();
    jcLoadIdentity();
    
    // Text (euclidean)
    if (!displaying.empty()) {
        jcColor4f(1,1,1,1);
        drawText(displaying.c_str(), -1/aspect + 0.1, 0.9, 0, false, false);	
        if (displaying_life) { displaying_life--; if (!displaying_life) displaying = ""; }
        
    }    
    
	if (blit)
		blit->display();
    
#if 1
	{ // Draw "interface"
//		goOrtho(); // Will be redundant if nothing since the beginning of the function has messed with the projection matrix.
//		glLoadIdentity();
//		glDisable(GL_DEPTH_TEST);

		cpSpaceHashEach(workingUiSpace()->staticShapes, &drawButton, NULL);

//		goPerspective();
	}
#endif
}

// ------------ AUDIO ---------------

#include "sound.h"

// Audio state:

#define NO_AUDIO 0
#define LOG_AUDIO SELF_EDIT

FILE *audiolog = NULL; // If you find yourself needing to debug your audio, fopen() this somewhere and all audio will start getting copied to this file as 16-bit pcm
int playing = 0; // So sounding can "pause"
int tapping = 1<<16; // "Silence"

#define DIGITALCLIP 1
#define CHIMEDURATION 1000

struct sounding { // I thought I was trying to use STL structures now...
    noise *from;
    sounding *next;
    int start;
    sounding(noise *_from, sounding *_next, int _start = 0) : from(_from), next(_next), start(_start) {}
    ~sounding() { delete from; }
};
sounding *soundroot;

// Audio code:

double * audio_load(const char *name, int *outlength = NULL, double amp = 1.0) {
    char filename[FILENAMESIZE]; internalPath(filename, name);
    FILE *file = fopen(filename, "r");
    long length = 0; fseek( file, 0, SEEK_END ); length = ftell( file );
    fseek( file, 0, SEEK_SET );
    length /= 2; // 16 bit pcm, 2 bytes per sample
    double *result = new double[length];
    short *result_raw = new short[length];
    fread(result_raw, sizeof(short), length, file);
    for(int c = 0; c < length; c++)
        result[c] = double(result_raw[c])/SHRT_MAX * amp;
    delete[] result_raw;
    if (outlength)
        *outlength = length;
    fclose(file);
    return result;
}

void audio_init() {
#if BEATS_DRUM // Doesn't even exist on windows.
    drumwhump = audio_load("JDrums029.raw", &drumwhump_l, 0.5);
#endif
    
#if LOG_AUDIO
	audiolog = fopen("/tmp/OUTAUDIO", "w");
#endif    
}

void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);
    
	for(int c = 0; c < slen; c++) {
		double value = 0;
                        
        for(sounding **n = &soundroot; *n;) {
            sounding *now = *n;
            
            if (playing < (*n)->start) {
                n = &(*n)->next; // Slightly duplicative of code
                continue;
            }
            
            now->from->to(value);
            
            if (now->from->done()) {
                sounding *victim = *n;
                *n = (*n)->next;
                delete victim;
            } else {
                n = &(*n)->next;
            }
        }
            
        playing++;
                
#if NO_AUDIO
		value = 0;
#endif

#if DIGITALCLIP
        if (value < -1) value = -1;
        if (value > 1) value = 1; // Maybe add like a nice gate function... I dunno
#endif
        
        // GARBAGE, REMOVE
        if (ticks > 1) {
            value = 1-(tapping/44100.0);
            if (value<0) value = 0;
        }
        
		samples[c] = value*SHRT_MAX;
        tapping++;
	}
	if (audiolog) { // For debugging audio
		fwrite(stream, sizeof(short), slen, audiolog);
	}
}
