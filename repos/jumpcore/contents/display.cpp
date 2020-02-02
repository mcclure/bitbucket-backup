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
#include "ent.h"

// Should this be avoided as an include?
#include "text_display.h"

#include "glCommon.h"
#include "glCommonMatrix.h"

#define RAINBOW_DEBUG 0
#define HORRIBLE_BROKEN 0

//texture_slice *interface_atlas;
//hash_map<string, texture_atlas *> atlas_root;

unsigned int randomColor() {
    return packColor((double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, 1);
}

unsigned int randomGray() {
	return packGray( random()/float(RANDOM_MAX) );
}

unsigned int packHsv(float h, float s, float v, float a) {
	double r,g,b;
	HSVtoRGB( &r, &g, &b, h, s, v );
	return packColor(r,g,b,a);
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
    
    ERR("Screen size %fx%f\n", (float)surfacew, (float)surfaceh);
    
    text_init();
    
	// Put your own initialization after this:
#if 0
    { 
        atlas_root = atlas_load("atlasDictionary.xml");
        texture_atlas *atlas = atlas_root["interface-1.png"];
        interface_atlas = atlas->parent;
		// TODO: grab subtextures here
	}
#endif
    
    glClearColor(0,0,0,1);
}

quadpile _scratch; int pushed = 0;

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
			_scratch.push4(verts, 0, (texture_slice *)body->data); pushed++;
		} break;
    }
}

// This is called when it is time to draw a new frame.
void display(void)
{
	if (!gl2) glEnableClientState(GL_VERTEX_ARRAY); // FIXME: If this line does not run in GL1 mode, nothing displays. If it *does* run on GL2 mode, the program crashes. Need to understand

	glClearColor(0,0,0,1); // NO NO NO BAD
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
#if 1
	glDisable(GL_DEPTH_TEST); // Currently, everything is 2D.
	goOrtho();
#else
//	glLineWidth(2);
//	glEnable(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST); // Wait, no. Currently, everything is 3D.
	goPerspective();
#endif
	
    jcLoadIdentity();
	
	global_ent->display(NULL);
    
    // Debug text line.. print to this with something like BANNER(FPS) << "Message"
    if (!displaying.empty()) {
        jcColor4f(1,1,1,1);
        drawText(displaying.c_str(), -1/aspect + 0.1, 0.9, 0, false, false);	
        if (displaying_life) { displaying_life--; if (!displaying_life) displaying = ""; }
        
    }
	
#if 0
	if (columns.size()) { // Draw "interface"-- checking columns.size is a cheesy way of checking if there's an interface right now
//		goOrtho(); // Will be redundant if nothing since the beginning of the function has messed with the projection matrix.
//		glLoadIdentity();
//		glDisable(GL_DEPTH_TEST);

		cpSpaceHashEach(workingUiSpace()->staticShapes, &drawButton, NULL);
	}
#endif
}

// ------------ AUDIO ---------------

#include <plaid/audio.h>
#include <plaid/audio/synth.h>

using namespace plaid;

Ref<Audio> audio;

// Audio code:

void audio_init() {
	audio = new Audio();
}

void audio_update() {
	audio->update();
	
}