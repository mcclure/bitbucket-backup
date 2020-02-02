// DISPLAY CODE

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
#include "cpRect.h"

#include "glCommon.h"
#include "glCommonMatrix.h"

#define RAINBOW_DEBUG 0

// TODO: THIS IS WAY TOO MUCH STATE! PUT THIS IN AN OBJECT OR SOMETHING
#define FCIRCLE_SLICES 3
texture_slice *interface_atlas;
subtexture_slice *fcircle[FCIRCLE_SLICES];
subtexture_slice *finter[ibar_max], *falter[ibar_max];
#define LETTER_COUNT 12
subtexture_slice *fletter[LETTER_COUNT];
subtexture_slice *farrow, *flarrow, *frarrow;

bool display_board = true, board_obscured = false, board_loading = false;

hash_map<string, texture_atlas *> atlas_root;

// much lke "scale"
const char *lettername[LETTER_COUNT] = {"a","as","b","c","cs","d","ds","e","f","fs","g","gs"};
hash_map<string, sample_inst *> library;

extern double radmin; // TODO move to program.h
extern int gs; // Current "global space"

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

#if TEXTUREPROCESS
subtexture_slice *rtt_slice = NULL;
GLuint rtt_fb = 0, rtt_rb = 0;
static int last_pcode = -1;

void splut_texture(subtexture_slice *s) {
	quadpile scratch2;
	scratch2.push(-1/aspect,-1,1/aspect,1, false, 1, s);
	scratch2.megaIndexEnsure(6);
	gl2SetState(s_texturebit + s_blurbit);
	glUniform1f(p->uniforms[s_blur], debug_floats[0]); // LAST FACTOR IS A PROBLEM!
	glBindTexture(GL_TEXTURE_2D, s->texture);			// Bind The Texture
	//	jcColor4f(1,1,1,1.0);
	int stride = VERT_STRIDE+TEX_STRIDE;
	jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch2[0]);
	jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &scratch2[0]+VERT_STRIDE);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);	
}

void CreateFrameBuffer(subtexture_slice *&slice, GLuint &fbo, GLuint &depthbuffer) {
	int height = surfaceh, width = surfacew;
	int ppower = 1;
	int psquare = 0;
	
	if (slice) {
		delete slice; slice = 0;
		jcDeleteFramebuffers(1, &fbo); fbo = 0;
		jcDeleteRenderbuffers(1, &depthbuffer); depthbuffer = 0;
	}
	
	vector<char> data;
	int side = 2; while (side < height || (!psquare && side < width)) side *= 2;
	side *= ppower;
	int size = side*side*4;
	data.resize(size);
	
	memset(&data[0], 0, size);
	//	for(int c = 0; c < size; c++) data[c] = 0;
	//	for(int x = 0; x < side; x++) for(int y = 0; y < side; y++) //Interference pattern
	//		((unsigned int *)&data[0])[x*side+y] = (x+y)%2?0xFF000000:0xFFFFFFFF;
	//	for(int x = 0; x < side; x++) for(int y = 0; y < side; y++) { //Gradient
	//		double shade = ::max(double(x)/side,double(y)/side); if (fmod(shade,0.1)>0.05) shade = 1-shade;
	//		((unsigned int *)&data[0])[x*side+y] = packColor(shade,shade,shade); }
	
	GLuint rttt = 0;
	glGenTextures(1, &rttt);					// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, rttt);			// Bind The Texture
	glTexImage2D(GL_TEXTURE_2D, 0, 4, side, side, 0, // GL_RGBA8 instead of 4 maybe?
				 GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);			// Build Texture Using Information In data
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);		
	// DESIRABLE?
#ifndef OPENGL_ES
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
#endif
	//	slice = new subtexture_slice(rttt, 0,0,double(surfacew)/side,double(surfaceh)/side);
	//	slice = new subtexture_slice(rttt, 0,0,ppower*double(psquare?surfaceh:surfacew)/side,ppower*double(surfaceh)/side);
	double squareoff = psquare?ppower*double(surfacew-surfaceh)/side/2:0;
	slice = new subtexture_slice(rttt, -squareoff,0,ppower*double(psquare?surfaceh:surfacew)/side+squareoff,ppower*double(surfaceh)/side);
	
	GLERR("RTT slice init");
	ERR("RTT with side %d yfact %lf\n", side, (double)slice->coords[3]);
	
	// TODO: Fix those constants
	jcGenFramebuffers(1, &fbo);
	jcBindFramebuffer(JC_FRAMEBUFFER, fbo);
	jcGenRenderbuffers(1, &depthbuffer);
	jcBindRenderbuffer(JC_FRAMEBUFFER, depthbuffer);
	jcRenderbufferStorage(JC_FRAMEBUFFER, GL_DEPTH_COMPONENT, side, side); // Should be width, height?
	jcFramebufferRenderbuffer(JC_FRAMEBUFFER, JC_DEPTH_ATTACHMENT, JC_RENDERBUFFER, depthbuffer);
	
	jcFramebufferTexture2D(JC_FRAMEBUFFER, JC_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rttt, 0);
	
	GLERR("RTT gen");
	
	GLenum status = jcCheckFramebufferStatusEXT(JC_FRAMEBUFFER);
	ERR("Framebuffer status %d (desired %d)\n", (int)status, (int)JC_FRAMEBUFFER_COMPLETE);
	
	jcBindFramebuffer(JC_FRAMEBUFFER, 0);
}
#endif

// Tarot specific

// "short name" for particular tarot card
string inameForTarot(int suit, int card) {
	const char *sn[SUITS] = {"ar", "cu", "pe", "sw", "wa"};
	char filenameshort[64];
	if (suit == 0) {
		snprintf(filenameshort, 64, "%s%02d", sn[suit], card);
	} else {
		const char *face = NULL;
		switch(card) {
			case 0:  face = "ac"; break;
			case 10: face = "pa"; break;
			case 11: face = "kn"; break;
			case 12: face = "qu"; break;
			case 13: face = "ki"; break;
		}
		if (face) {
			snprintf(filenameshort, 64, "%s%s", sn[suit], face);
		} else {
			snprintf(filenameshort, 64, "%s%02d", sn[suit], card+1);
		}
	}
	return filenameshort;
}

// filename path for card
string ipnameForTarot(int suit, int card) {
	char filenamespec[64];
	string shortname = inameForTarot(suit, card);
	char filename2[FILENAMESIZE];
	snprintf(filenamespec, 64, "%s.jpg", shortname.c_str());
	internalPath(filename2, filenamespec);
	return filename2;
}

subtexture_slice *imgTarot[SUITS][MAJORS];
sample_inst *allTarot[SUITS][MAJORS];

sample_inst *instForTarot(int suit, int card) {
	return allTarot[suit][card];
}

// ------------- OPENGL WRAPPERS ------------------

subtexture_slice *chip_payload::icon() {
	if (samp && samp->icon) return samp->icon;
	if (doLetters) {
		int l = p % LETTER_COUNT; if (l < 0) l += LETTER_COUNT;
		return fletter[l];
	}
	return fcircle[0];
}

double chip_payload::h() { // Hue
//	const double top = (360.0*11/12);
//	return top - double(w-1)/(200)*top;
	return ::min( (p-NOTE_BASE)*((360/36.0) * (4/6.0)), 300.0 );
}

bool chip_payload::oversize() {
	return samp && samp->icon && samp->icon->rx && samp->icon->rx == 17;
}

generator_bb::generator_bb(chip_payload _ch) : barbutton(0,0), ch(_ch) {
	double r, g, b, h = ch.h(), s = 1, v = 1;
	HSVtoRGB(&r,&g,&b,h,s,v);
	this->s = ch.icon();
	color = packColor(r,g,b);
}

// This is called once each time the display surface is initialized. It's a good place to do things like initialize
// fonts and textures. (Note it could be called more than once if the window size ever changes.)
void display_init(bool reinit) {
    for(int c = 0; c < GLCS_LAST; c++) glcsIs[c] = -1;
    for(int c = 0; c < GLE_LAST; c++) gleIs[c] = -1;
    jcMatrixInit();
    
    ERR("Screen size %fx%f", (float)surfacew, (float)surfaceh);
    glPixel = 2.0/surfaceh;

	if (surfaceh < 500 && surfacew < 500) { // FIXME: Will not catch iPhone 4?
#ifdef TARGET_MOBILE
		phoneFactor = true;	
#endif
#if 1
		double sizeStep = glPixel*16;
		chipSize = fmod(BASE_CHIP + sizeStep/2, sizeStep);
		sizeStep = glPixel*17;
		chipSizePlus = fmod(BASE_CHIP + sizeStep/2, sizeStep);
#endif
	}
	
    text_init();
	setupDrawTextWrapped(2/aspect * 0.9);
    
    { char file[FILENAMESIZE]; internalPath(file, "numbers.png");
        debug_numbers_slice = new slice(); debug_numbers_slice->load(file); }

    { 
        atlas_root = atlas_load("atlasDictionary.xml");
        texture_atlas *atlas = atlas_root["interface-1.png"];
        interface_atlas = atlas->parent;
        
        fcircle[0] = atlas->images["circle1"];
        fcircle[1] = atlas->images["circle2"];
        fcircle[2] = atlas->images["circle3"];
        
        finter[ibar_esc]      = atlas->images["esc"];
        finter[ibar_save]     = atlas->images["save"];
        finter[ibar_load]     = atlas->images["load"];
        finter[ibar_reset]    = atlas->images["reset"];
        finter[ibar_dupe]     = atlas->images["dupe"];
        finter[ibar_grid]     = atlas->images["grid"];
        finter[ibar_settings] = atlas->images["settings"];
		
		falter[ibar_dupe] = atlas->images["drag"];
		falter[ibar_grid] = atlas->images["nogrid"];
		
		farrow = atlas->images["downarrow"];
		flarrow = atlas->images["leftarrow"];
		frarrow = atlas->images["rightarrow"];

		for(int c = 0; c < LETTER_COUNT; c++) {
			fletter[c] = atlas->images[lettername[c]];
		}
    }
	
	{
		const char *genericname = "toml";
		sample_inst *toml = library_check(genericname, 0, NULL);
		for(int suit = 0; suit < SUITS; suit++) {
			for(int card = 0; card < MAJORS; card++) {
				if (!suit || card < MINORS) {
					sample_inst *i = new sample_inst(genericname);
					texture_slice *s = new texture_slice();
					string ipn = ipnameForTarot(suit, card);
					ERR("LOAD TAROT %s\n", ipn.c_str());
					s->load(ipn.c_str());
					subtexture_slice *rs = s->sub();
					imgTarot[suit][card] = rs;
					i->pmod = 0;
					i->samp = toml->samp;
					i->icon = rs;
					i->suit = suit;
					i->card = card;
					allTarot[suit][card] = i;
				} else {
					allTarot[suit][card] = NULL;
					imgTarot[suit][card] = NULL;
				}
			}
		}
	}
	
#if TEXTUREPROCESS
	if (PROCESSSAFE) {
		CreateFrameBuffer(rtt_slice, rtt_fb, rtt_rb);
	}	
#endif
    
    glClearColor(0,0,0,1);
//    glEnable(GL_LINE_SMOOTH);  // Attractive!
//    glEnable(GL_POINT_SMOOTH); // Ugly.
}

cpVect noturn = cpvforangle(0);
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

void outline_shape_start(cpBody *body, void *ptr, cpVect *topLeft, cpVect *bottomRight) {
	clear_lines();
	cpPolyShape *poly = (cpPolyShape *)ptr;
	for(int c = 0; c < poly->numVerts; c++) {
		_scratch_lines.push(cpvadd(body->p, poly->verts[c]), cpvadd(body->p, poly->verts[(c+1)%poly->numVerts]), true);
		pushed_lines++;
		if (topLeft && c == 1) *topLeft = cpvadd(body->p, poly->verts[c]); // Unacceptable
		if (bottomRight && c == 3) *bottomRight = cpvadd(body->p, poly->verts[c]);
	}	
}

void outline_shape(cpBody *body, void *ptr) {
	outline_shape_start(body, ptr, NULL, NULL);
	display_lines();
}

enum { DISPLAYING_GUIDES = 0, DISPLAYING_TAROT, DISPLAYING_HOLD, DISPLAYING_MAX };
unsigned int displaying_what = 0;

void display_object(void *ptr, void *data) {
    cpShape *shape = (cpShape*)ptr;
    cpBody *body = shape->body;
    
    switch(shape->collision_type) {
        case C_CHIP_OUTER: {
			cpPolyShape *poly = (cpPolyShape *)ptr;
            double h,s;
            subtexture_slice *drawing = NULL;
			
			extern cpShape *lastShapeTouched;
			unsigned int displaying_need = (ptr == lastShapeTouched ? DISPLAYING_HOLD : DISPLAYING_TAROT);
			if (displaying_what != displaying_need) break;
			
			// There's this ugly hack where the OUTER circle is oversized on phones, so we have to ignore
			// the OUTER's size on those devices. What we really ought to do is make the inner circle be
			// the one used to draw do that we won't have to correct when OUTER changes. Used to INNER was
			// used for collision handling but collisions are disabled now.
			double r = fabs(poly->verts[0].y);
			
            if (body->data) {
#define FIREHOWLONG (FPS/8)
                guichip *ch = (guichip *)body->data;
                int age = ticks-ch->fired;
				h = ch->ip.h();
                s = age < FIREHOWLONG ? double(age)/FIREHOWLONG : 1;
				drawing = ch->ip.icon();
#if 0
				r = !ch->oversize ? chipSize : chipSizePlus;
#endif				
				
            } else {
                h = 0; s = 0; drawing = fcircle[0];
            }
			            
            simpleDrawSprite(body->p.x, body->p.y, r, drawing);
        } break;
            
        case C_BAR: {
            bardata *bd = (bardata *)body->data;
            if (!bd || board_obscured)
                break;
            
			if (displaying_what != DISPLAYING_GUIDES) break;
			
			cpVect topLeft = cpvzero, bottomRight = cpvzero;
			outline_shape_start(body, ptr, &topLeft, &bottomRight);
            
            int bcount = bd->button_count();
            if (!bcount)
                break;
            
            cpFloat across = (bottomRight.x-topLeft.x)/bcount;
            cpFloat halfdown = (bottomRight.y-topLeft.y)/2;
            cpFloat halfdown_context = (bottomRight.y + topLeft.y)/2;
            
            for(int c = 0; c < bcount; c++) {
                cpFloat to = topLeft.x + across*c;
                _scratch_lines.push(to, topLeft.y, to, bottomRight.y, true);
                pushed_lines++;
                
				barbutton *b = bd->button(c);
				subtexture_slice *s = b->icon();
				
				if (s) {
					double offx = to + across/2;
					double offy = halfdown_context;
					double size = halfdown*2/3;
					
					clear_scratch();
					scratch_texture = s->texture;
					_scratch.push(offx+size,offy-size,offx-size,offy+size, true, 0xFFFFFFFF, s); pushed++;
					display_scratch();
				}
            }
			
			if (bd->border)
				display_lines();
			else 
				clear_lines();
        } break;

        case C_GUIDE: {
			if (!do_grid) break;
			if (displaying_what != DISPLAYING_GUIDES) break;
			outline_shape(body, ptr);
        } break;
			
		case C_TEXTBOX: { 
			if (displaying_what != DISPLAYING_GUIDES) break;
			textboxdata *td = (textboxdata *)body->data;
			drawTextWrapped(td->contents, body->p.x, body->p.y, true);
		} break;
    }
}

// This is called when it is time to draw a new frame.
void display_game(void)
{	
    EnableClientState(GLCS_VERTEX); // Only relevant to GL1-- maybe shouldn't be here at all

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	goOrtho();
    jcLoadIdentity();
	glDisable(GL_DEPTH_TEST);
    	
    // Text (euclidean)
    if (!displaying.empty()) {
        jcColor4f(1,1,1,1);
		double width = textWidth(displaying);
        drawText(displaying.c_str(), -width/2, 0.9, 0, false, false);	
        if (displaying_life) { displaying_life--; if (!displaying_life) displaying = ""; }
    }    
    
	// Build arrays
	clear_scratch();
	clear_lines();
#if !TWIST1_DEBUG
	clear_points();
#endif
	
	if (display_board) {
	// Feed space objects into arrays:
	{
		spaceinfo *s = &(spaces[gs]);

		for(displaying_what = 0; displaying_what < DISPLAYING_MAX; displaying_what++) {
			cpSpaceHashEach(s->space->staticShapes, &display_object, s);
			cpSpaceHashEach(s->space->activeShapes, &display_object, s);
		}

		// Debug code-- feel free to remove completely.
#if DRAW_DEBUG
        States(false, false);
        
        // Todo: Should be also behind DRAW_DEBUG
		cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
		cpSpaceHashEach(s->space->activeShapes, &drawObject, s);
				
		if (optDebug) {
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
	
#if 0
	// Arrays are built, now draw points:
	display_points();
		
	// Then polys:
	display_scratch();
		
	// Then lines:
	display_lines();
#endif
	} // display_board
        
#if TWIST1_DEBUG
	display_points();
#endif
}

void display(void) {
	bool process = true;
	int ppower = 1;
	int psquare = 0;
	
#if !TEXTUREPROCESS
	display_game();
#else
	bool doprocess = PROCESSSAFE && process;
	double cached_aspect = aspect;
	if (doprocess) {
		jcBindFramebuffer(JC_FRAMEBUFFER, rtt_fb);
		if (psquare) {
			cached_aspect = aspect;
			aspect = 1;	
			rinseGl();
		}
		glViewport(0,0,(psquare?surfaceh:surfacew)*ppower,surfaceh*ppower);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DNCOMMIT
	}
	display_game();
	if (doprocess && rtt_slice) {
		if (psquare)
			aspect = cached_aspect;
		jcBindFramebuffer(JC_FRAMEBUFFER, 0);
		glViewport(0,0,surfacew,surfaceh);
		splut_texture(rtt_slice);
	}
#endif
	
	// Interface is drawn separately [less efficiently]
#if 1
	if (board_obscured) {
		States(false, false);
		quadpile scratch2;
		scratch2.push(-1/aspect,-1,1/aspect,1, false);
		scratch2.megaIndexEnsure(6);
		jcColor4f(0,0,0,0.8);
		int stride = VERT_STRIDE;
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch2[0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);	
	}
	
	if (columns.size()) { // Draw "interface"-- checking columns.size is a cheesy way of checking if there's an interface right now
		//		goOrtho(); // Will be redundant if nothing since the beginning of the function has messed with the projection matrix.
		//		glLoadIdentity();
		//		glDisable(GL_DEPTH_TEST);
		
		cpSpaceHashEach(workingUiSpace()->staticShapes, &drawButton, NULL);
	}
#endif	
}

#if TWIST1_DEBUG
void twisthit(cpVect v) {
	ERR("TEST %d: %lf, %lf\n", ticks, (double)v.x, (double)v.y);
	_scratch_points.push(v);
	pushed_points++;
}
#endif

// ------------ AUDIO ---------------

#include "sound.h"

// Audio state:

#define NO_AUDIO 0
#define LOG_AUDIO SELF_EDIT



FILE *audiolog = NULL; // If you find yourself needing to debug your audio, fopen() this somewhere and all audio will start getting copied to this file as 16-bit pcm
bool metro_dirty = false;
cpVect metro_at_drawn = cpvzero;
bool metro_special_happening = false;

// Audio code:	

// Not endian safe	
#ifndef TARGET_MOBILE
double * wav_load(const char *name, int *outlength = NULL, bool internal = true, double amp = 1.0) {
    char filename[FILENAMESIZE]; 
	
	if (internal)
		internalPath(filename, name);
	else
		strncpy(filename, name, FILENAMESIZE);	
	
	FILE *file = fopen(filename, "rb");
	if (!file) return NULL;

	uint32_t tag = 0, chunksize = 0;
	fread(&tag, sizeof(tag), 1, file);
	if (tag != 'FFIR') {
		ERR("INVALID WAV-- NO RIFF\n");
		fclose(file);
		return NULL;
	}
	fseek( file, 4, SEEK_CUR ); // Skip filesize, we don't care
	fread(&tag, sizeof(tag), 1, file);
	if (tag != 'EVAW') {
		ERR("INVALID WAV-- NO WAVE\n");
		fclose(file);
		return NULL;
	}
	
	do { // Surf RIFFs
		bool success = 1 == fread(&tag, sizeof(tag), 1, file);
		if (success) success = 1 == fread(&chunksize, sizeof(chunksize), 1, file);
		if (!success) {
			ERR("INVALID WAV-- NO DATA\n");
			fclose(file); 
			return NULL;
		}
		ERR("SURF... %c%c%c%c size %d\n", ((char *)&tag)[0], ((char *)&tag)[1], ((char *)&tag)[2], ((char *)&tag)[3], chunksize);
		if (tag == 'atad') // We found it
			break;
		fseek( file, chunksize, SEEK_CUR );
	} while(1);
	
	long length = chunksize;
	length /= 2; // 16 bit pcm, 2 bytes per sample
	double *result = new double[length];
	short *result_raw = new short[length];
	memset(result_raw, length*sizeof(short), 0); // Prepare in case of invalid wav
	fread(result_raw, sizeof(short), length, file);
	for(int c = 0; c < length; c++)
		result[c] = double(result_raw[c])/SHRT_MAX * amp;
	delete[] result_raw;
	if (outlength)
		*outlength = length;
	fclose(file);
	return result;
}

// Not endian safe	
void wav_write(const char *filename) {
	const int samples = mainCircle.samplesPerLoop();
	const int buffersize = samples*2;
	const int chunksize = 256;
	
	circle dupeCircle(mainCircle);
	dupeCircle.nowat = 0.75; // In case the user thinks the "start" of the pattern is the top.
	
	uint16_t *buffer = new uint16_t[buffersize];
	for(int progress = 0; progress < buffersize; progress += chunksize) {
		audio_callback(&dupeCircle, (uint8_t *)(buffer + progress), min(buffersize - progress, chunksize)*sizeof(uint16_t));
	}
	
	FILE *file = fopen(filename, "wb");
	{ // See https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
		uint32_t d4; uint16_t d2;
		const int samplerate = 44100;
		const int channels = 1;
		const int samplesize = sizeof(uint16_t);
#define W4( x ) d4 = (x); fwrite(&d4, sizeof(d4), 1, file);
#define W2( x ) d2 = (x); fwrite(&d2, sizeof(d2), 1, file);
#define WS( x ) fwrite((x), 1, 4, file);
		
		WS("RIFF");
		W4(36 + samples*samplesize); // Data + header
		WS("WAVE");
		WS("fmt ");
		W4(16); // Magic number -- WAVE section size
		W2(1); // Magic [indicates PCM]
		W2(channels); // # of channels
		W4(samplerate); // Sample rate
		W4(samplerate * channels * samplesize); // Bytes/sec
		W2(channels * samplesize); // Block alignment ?? wav is stupid
		W2(samplesize*8);
		WS("data");
		W4(samples*samplesize);
	}
	fwrite(buffer + samples, sizeof(short), samples, file);
	fclose(file);
		   
	delete[] buffer;
}	

#endif
	
double * audio_load(const char *name, int *outlength = NULL, bool internal = true, double amp = 1.0) {
    char filename[FILENAMESIZE]; 
	
	if (internal)
		internalPath(filename, name);
	else
		strncpy(filename, name, FILENAMESIZE);	
	
    FILE *file = fopen(filename, "r");
	if (!file) return NULL;
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

sample_inst *library_check(const char *name, int pmod, const char *img) {
	static texture_atlas *atlas = NULL;
	sample_inst *result;
	if (library.count(name)) {
		result = library[name];
	} else {
		result = new sample_inst(name);
		result->pmod = pmod;
		string fullname = result->name + ".wav"; // Try to load external wav first
#ifndef TARGET_MOBILE
		result->samp = wav_load(fullname.c_str(), &result->len, false);
#endif
		if (!result->samp) {
			fullname = result->name + ".raw"; // No wav, look for a raw
#ifndef TARGET_MOBILE
			// Prefer to load from working directory over internally
			result->samp = audio_load(fullname.c_str(), &result->len, false);
#endif
			if (!result->samp) // Still nothing-- just load internally
				result->samp = audio_load(fullname.c_str(), &result->len);
		}
		library[name] = result;
	}
	// There is an interesting minor bug in this next bit. Because as soon as library_check returns
	// a generator_bb will most likely be created, icon will be read as soon as this returns. So if
	// we process an xml with no-specified-icon before the xml which specifies the icon, the icon will
	// not make it back in to the earlier-processed xml's generator_bb-- UNLESS the key is changed, in
	// which case all generator_bbs get reloaded! Now that is just unpleasant. And hard to explain.
	// This could be largely addressed by making sure samples.xml is always processed first...
	if (img && !result->icon) {
		if (!atlas) atlas = atlas_root["interface-1.png"]; // I don't like this (the use of a literal constant I mean).
		if (atlas->images.count(img))
			result->icon = atlas->images[img];
	}
	return result;
}

void audio_init() {
#if LOG_AUDIO
	audiolog = fopen("/tmp/OUTAUDIO", "w");
#endif    
}