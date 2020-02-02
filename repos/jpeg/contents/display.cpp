// DISPLAY CODE

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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "kludge.h"

#include "chipmunk.h"
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
#define HORRIBLE_BROKEN 0

texture_slice *interface_atlas;
hash_map<string, texture_atlas *> atlas_root;

unsigned int randomColor() {
    return packColor((double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, 1);
}

// Protect with a #?
string displaying;
int displaying_life = 0;

// ------------- OPENGL CODE ------------------

// This is called once each time the display surface is initialized. It's a good place to do things like initialize
// fonts and textures. (Note it could be called more than once if the window size ever changes.)
// FIXME: At present there is a small memory leak here on each re-call of display_init
void display_init(bool reinit) {
	rinseGl();
    jcMatrixInit();
    
    ERR("Screen size %fx%f", (float)surfacew, (float)surfaceh);
    
    text_init();
    
	// Put your own initialization after this:

    { 
        atlas_root = atlas_load("atlasDictionary.xml");
        texture_atlas *atlas = atlas_root["interface-1.png"];
        interface_atlas = atlas->parent;
		// TODO: grab subtextures here
	}
    
    glClearColor(0,0,0,1);
}

quadpile _scratch; int pushed = 0;
linepile _scratch_lines; int pushed_lines = 0;
pile _scratch_points; int pushed_points = 0;
int scratch_texture = 0;

void clear_scratch() { 	_scratch.clear(); pushed = 0; }
void clear_lines() { _scratch_lines.clear(); pushed_lines = 0; }
void clear_points() { _scratch_points.clear(); pushed_points = 0; }

void display_points() {
	if (!gl2 && pushed_points) {
		States(false, false);
		int stride = VERT_STRIDE;
		jcColor4f(1.0/3, 1.0/3, 1.0/3, 1.0);
		glPointSize(8);
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &_scratch_points[0]);
		glDrawArrays(GL_POINTS, 0, pushed_points);                    
	}	
}

void display_scratch() {
	if (pushed) {
		States(true, true);
		_scratch.megaIndexEnsure(6*pushed);
		int stride = VERT_STRIDE+COLOR_STRIDE+TEX_STRIDE;
		glBindTexture(GL_TEXTURE_2D, scratch_texture);
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &_scratch[0]);
		jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &_scratch[0]+VERT_STRIDE);
		jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &_scratch[0]+VERT_STRIDE+COLOR_STRIDE);
		glDrawElements(GL_TRIANGLES, 6*pushed, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);        	
	}
}

void display_lines() {
	if (pushed_lines) {
		States(false, true);
		int stride = VERT_STRIDE+COLOR_STRIDE;
		glLineWidth(4);
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &_scratch_lines[0]);
		jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &_scratch_lines[0]+VERT_STRIDE);
		glDrawArrays(GL_LINES, 0, 2*pushed_lines);	
	}
}

void simpleDrawSprite(double offx, double offy, double size, subtexture_slice *icon) {
    offx -= size; offy += size; size *= 2; // Stupid... opengl.. coordinates.. whatever
    double r = 1, g = 1, b = 1;
	
	clear_scratch();
	scratch_texture = icon->texture;
    _scratch.push(offx,offy,offx+size,offy-size, r, g, b, 1.0, icon); pushed++;    
	display_scratch();
}

void display_object(void *ptr, void *data) {
    cpShape *shape = (cpShape*)ptr;
    cpBody *body = shape->body;
	
    switch(shape->collision_type) {
		// Do whatever here
		
		case C_CLUTTER: { // Assumes  we did in fact create that body with 4 vertices...
			cpPolyShape *poly = (cpPolyShape *)shape;
			cpVect verts[4];
			for(int c = 0; c < 4; c++)
				verts[c] = cpvadd(body->p, cpvrotate(poly->verts[c], body->rot));
			_scratch.push4(verts, false, 0, (subtexture_slice *)body->data); pushed++;
		} break;
    }
}

// This is called when it is time to draw a new frame.
void display(void)
{	
    EnableClientState(GLCS_VERTEX); // Only relevant to GL1-- maybe shouldn't be here at all

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	goOrtho();
    jcLoadIdentity();

	if (machine && machine->displayInfo) {
		ostringstream o;
		
		if (!machine->f) {
			o << "No such file.";
		} else {
			o << machine->f->patches.size() << " patches";
			if (!machine->subtexture || !machine->rawtexture->constructed)
				o << "\nFAIL";
		}
		
        jcColor4f(1,1,1,1);
        drawText(o.str().c_str(), -1/aspect + 0.1, 0.9, 0, false, false);	      
    }    	
	
#if 0 // No actual chipmunk step
    // Debug text line.. print to this with something like BANNER(FPS) << "Message"
    if (!displaying.empty()) {
        jcColor4f(1,1,1,1);
        drawText(displaying.c_str(), -1/aspect + 0.1, 0.9, 0, false, false);	
        if (displaying_life) { displaying_life--; if (!displaying_life) displaying = ""; }        
    }    

    // Print each space in the stack
	for(int c = spaces.size()-1; c >= 0; c--) { // FIXME: Forward or backward?
		spaceinfo *s = &(spaces[c]);
		
		glDisable(GL_DEPTH_TEST); // You probably do not want to disable depth test.

        _scratch.clear(); pushed = 0;
		cpSpaceHashEach(s->space->staticShapes, &display_object, s);
		cpSpaceHashEach(s->space->activeShapes, &display_object, s);

		// Debug code-- feel free to remove completely. Switch on in display.h
#if DRAW_DEBUG
		if (optDebug) {
			
			cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
			cpSpaceHashEach(s->space->activeShapes, &drawObject, s);

			// Draw positions of bodies.
			cpArray *bodies = s->space->bodies;
			
			glBegin(GL_POINTS); {
				for(int i=0; i<bodies->num; i++){
					cpBody *body = (cpBody*)bodies->arr[i];
                    glColor3f(1, 1, 1);
					glVertex2f(body->p.x, body->p.y);
				}
//				glColor3f(1.0, 0.0, 0.0);
//				cpArrayEach(s->space->arbiters, &drawCollisions, NULL);
			} glEnd();
			
#if AXIS_DEBUG
			glBegin(GL_LINES); { 
				for(int i=0; i<bodies->num; i++){
					cpBody *body = (cpBody*)bodies->arr[i];
					glColor3f(1.0, 0.0, 1.0);
					glVertex2f(body->p.x, body->p.y);
					glVertex2f(body->p.x+body->rot.x*36, body->p.y+body->rot.y*36);
				}
			} glEnd();
#endif		
		}
#endif
    }
	
	{
		// display_object earlier will have filled out _scratch with a vertex array; we now just draw it
		States(true, false);
		_scratch.megaIndexEnsure(6*pushed);
		int stride = VERT_STRIDE+TEX_STRIDE;
		glBindTexture(GL_TEXTURE_2D, interface_atlas->texture);
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &_scratch[0]);
		jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &_scratch[0]+VERT_STRIDE);
		glDrawElements(GL_TRIANGLES, 6*pushed, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);        		
	}
#endif
	
	if (machine && machine->subtexture) {
		simpleDrawSprite(0, 0, 1, machine->subtexture);
	}
	
#if 1
	if (columns.size()) { // Draw "interface"-- checking columns.size is a cheesy way of checking if there's an interface right now
//		goOrtho(); // Will be redundant if nothing since the beginning of the function has messed with the projection matrix.
//		glLoadIdentity();
//		glDisable(GL_DEPTH_TEST);

		cpSpaceHashEach(workingUiSpace()->staticShapes, &drawButton, NULL);
	}
#endif
}

// ------------ AUDIO ---------------

#include "sound.h"

// Audio state:

#define NO_AUDIO 0
//#define LOG_AUDIO SELF_EDIT

FILE *audiolog = NULL; // If you find yourself needing to debug your audio, fopen() this somewhere and all audio will start getting copied to this file as 16-bit pcm
int playing = 0; // So sounding can "pause"
int tapping = 0;
bool toggle_fire; // Used in demo, feel free to remove

#define DIGITALCLIP 1
#define CHIMEDURATION 5000

struct sounding { // I thought I was trying to use STL structures now...
    noise *from;
    sounding *next;
    int start;
    sounding(noise *_from, sounding *_next, int _start = 0) : from(_from), next(_next), start(_start) {}
    ~sounding() { delete from; }
};
sounding *soundroot;

// Audio code:

// Use at your own risk
double * audio_load(const char *name, int *outlength = NULL, double amp = 1.0) {
    char filename[FILENAMESIZE]; internalPath(filename, name);
    FILE *file = fopen(filename, "r"); // REALLY SHOULD DO SOMETHING IF THIS RETURNS NULL
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
#if LOG_AUDIO
	audiolog = fopen("/tmp/OUTAUDIO", "w");
#endif    
}

// Len is in bytes
void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);
    
#if 1
    if (toggle_fire) {
        soundroot = new sounding( new notone(CHIMEDURATION, (double)random()/RANDOM_MAX * 30 + 1, 0.1),
                                 soundroot, 0);
        toggle_fire = false;
    }
#endif
    
	for(int c = 0; c < slen; c++) { // Write your audio code in here. My default just fires a simple list of "sounding" objects:
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
        
		samples[c] = value*SHRT_MAX;
        tapping++;
	}
	if (audiolog) { // For debugging audio
		fwrite(stream, sizeof(short), slen, audiolog);
	}
}
