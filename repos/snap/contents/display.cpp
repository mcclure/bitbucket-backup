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
#define VELOCITY_MAX 0 // 5 // Pass off to shader somehow maybe?
#define VELOCITY(x) ((x+5)/double(VELOCITY_MAX*2))
#define TEXTURED_PLAYER 1
#define GLOWY_PLAYER 1

texture_slice *interface_atlas, *col40, *glow_atlas;

hash_map<string, texture_atlas *> atlas_root;
#if TEXTURED_PLAYER
subtexture_slice *p2_s;
#endif
subtexture_slice *box_s[3], *box_glow[3];

display_idiom *di = new display_idiom();
level_idiom *li = new level_idiom();
double debug_floats[3] = {0,0,0}; // Nonsense

#if TEXTUREPROCESS
subtexture_slice *rtt_slice[2] = {NULL,NULL};
GLuint rtt_fb[2] = {0,0}, rtt_rb[2] = {0,0};
static int last_pcode = -1;

#define ARENA_INSET 0.025
#define TIMEBAR_HEIGHT 0.05
#define HEALTHBAR_RAD (BULLET_RAD)
#define HEALTH_RAD (BULLET_RAD/2.0)

cpRect screen;
cpRect arena;
int arenah, arenaw;
int col40w, col40h; double col40wp, col40hp;

void splut_texture(subtexture_slice *s, bool blur, cpRect at) {
	quadpile scratch2;
	scratch2.push4(at, false, 0, s);
	scratch2.megaIndexEnsure(6);
	gl2SetState(s_texturebit + s_blurbit);
	glUniform1f(p->uniforms[s_blur], debug_floats[0]);
	glBindTexture(GL_TEXTURE_2D, s->texture);			// Bind The Texture
	//	jcColor4f(1,1,1,1.0);
	int stride = VERT_STRIDE+TEX_STRIDE;
	jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch2[0]);
	jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &scratch2[0]+VERT_STRIDE);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);	
}

void CreateFrameBuffer(subtexture_slice *&slice, GLuint &fbo, GLuint &depthbuffer) {
	int height = arenah, width = arenaw;
	
	if (slice) {
		delete slice; slice = 0;
		jcDeleteFramebuffers(1, &fbo); fbo = 0;
		jcDeleteRenderbuffers(1, &depthbuffer); depthbuffer = 0;
	}
	
	vector<char> data;
	int side = 2; while (side < height || (side < width)) side *= 2;
	side *= di->ppower;
	int size = side*side*4;
	data.resize(size);
	
	memset(&data[0], 0, size);
	//	for(int c = 0; c < size; c++) data[c] = 0;
		for(int x = 0; x < side; x++) for(int y = 0; y < side; y++) //Interference pattern
			((unsigned int *)&data[0])[x*side+y] = (x+y)%2?0xFF000000:0xFFFFFFFF;
	//	for(int x = 0; x < side; x++) for(int y = 0; y < side; y++) { //Gradient
	//		double shade = ::max(double(x)/side,double(y)/side); if (fmod(shade,0.1)>0.05) shade = 1-shade;
	//		((unsigned int *)&data[0])[x*side+y] = packColor(shade,shade,shade); }
	
	GLuint rttt = 0;
	glGenTextures(1, &rttt);					// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, rttt);			// Bind The Texture
	glTexImage2D(GL_TEXTURE_2D, 0, 4, side, side, 0, // GL_RGBA8 instead of 4 maybe?
				 GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);			// Build Texture Using Information In data
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); // NEAREST/LINEAR DEPENDING ON V
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);		
	// DESIRABLE?
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	//	slice = new subtexture_slice(rttt, 0,0,double(surfacew)/side,double(surfaceh)/side);
	//	slice = new subtexture_slice(rttt, 0,0,di->ppower*double(di->psquare?surfaceh:surfacew)/side,di->ppower*double(surfaceh)/side);
	slice = new subtexture_slice(rttt, 0,0,di->ppower*double(width)/side,di->ppower*double(height)/side);
	
	GLERR("RTT slice init");
	ERR("RTT with width %d, height %d = side %d\n", width, height, side);
	
	// TODO: Fix those constants
	jcGenFramebuffers(1, &fbo);
	jcBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
	jcGenRenderbuffers(1, &depthbuffer);
	jcBindRenderbuffer(GL_RENDERBUFFER_EXT, depthbuffer);
	jcRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, side, side); // Should be width, height?
	jcFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
	
	jcFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, rttt, 0);
	
	GLERR("RTT gen");
	
	GLenum status = jcCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	ERR("Framebuffer status %d (desired %d)\n", (int)status, (int)GL_FRAMEBUFFER_COMPLETE_EXT);
	
	jcBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
}
#endif

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
void display_init() {
	// Gonna need an ifdef TARGET_DESKTOP someday?
	SDL_ShowCursor(false);
	
	rinseGl();
    jcMatrixInit();
    
    ERR("Screen size %fx%f", (float)surfacew, (float)surfaceh);
    
    text_init();
    
	// Put your own initialization after this:

    { 
        atlas_root = atlas_load("atlasDictionary.xml");
        texture_atlas *atlas = atlas_root["interface-1.png"];
        interface_atlas = atlas->parent;
		
#if TEXTURED_PLAYER
		p2_s = atlas->images["player_over"];
#endif
		box_s[0] = NULL; //atlas->images["box_ammo"];
		box_s[1] = atlas->images["box_health"];
		box_s[2] = NULL; //atlas->images["box_snap"];
		
		atlas = atlas_root["glows-1.png"];
		glow_atlas = atlas->parent;
		
		box_glow[0] = atlas->images["blur8"];
		box_glow[1] = atlas->images["blur16"];
		box_glow[2] = atlas->images["blur32"];
	}
	
	{
		char file[FILENAMESIZE]; internalPath(file, "english.png");
        col40 = new texture_slice(); col40->load(file);
	}
    
    glClearColor(0,0,0,1);
	
	float timebar_offset = TIMEBAR_HEIGHT*2;
	screen = cpr(cpvzero, cpv(1/aspect, 1));
	arena = screen;

	arena = arena.set_l( 0 );
	arena = arena.set_u( arena.u() - timebar_offset );
	arena = arena.set_r( arena.r() - HEALTHBAR_RAD*2 );
	arena = arena.inset_fixed(ARENA_INSET);
	arena = arena.set_l( HEALTHBAR_RAD/2 ); // Correct relative to health bars
//	arena_size.x -= HEALTHBAR_RAD*2;// arena_origin.x += HEALTHBAR_RAD; // Wait why can't I do this?
	
	arenaw = ceil(arena.rad.x / (1/aspect) * surfacew);
	arenah = ceil(arena.rad.y / (1) * surfaceh);
	
	{ // Only some of this gets used?
		int xbase = 7*40, ybase = 8*24;
		int xfactor = surfacew / xbase, yfactor = surfaceh / ybase;
		int usefactor = ::min(xfactor,yfactor);
		col40w = xbase * usefactor; col40h = ybase * usefactor;
		col40wp = double(col40w)/surfacew; col40hp = double(col40h)/surfaceh;
		ERR("xf %d yf %d uf %d c40w %d c40h %d wp %lf hp %lf\n", xfactor, yfactor, usefactor, col40w, col40h, col40wp, col40hp);
	}
	
#if TEXTUREPROCESS
	if (PROCESSSAFE && last_pcode != di->pcode()) {
		CreateFrameBuffer(rtt_slice[0], rtt_fb[0], rtt_rb[0]);
#if DOUBLEPROCESS
		glActiveTexture ( GL_TEXTURE1 ); GLERR("ActiveTexture2");
		CreateFrameBuffer(rtt_slice[1], rtt_fb[1], rtt_rb[1]);
		glActiveTexture ( GL_TEXTURE0 ); GLERR("ActiveTexture3");
#endif
		last_pcode = di->pcode();
	}
#endif		
}

#define SIZEMOD_Q  2
#define SIZEMOD_LI 3
#define SIZEMOD_T  16 /* Wrong! */
#define SIZEMOD_TC 20 /* Also wrong! */

struct draw_all {
	quadpile q;  // Quads
	quadpile t;	 // Textured quads
	linepile li; // Lines
	unsigned int texture; bool tcolor;
	void clear() { q.clear(); t.clear(); li.clear(); tcolor = false; }
};

#if DOUBLEPROCESS
#define SCRATCHES 2
#else
#define SCRATCHES 3
#define V_SCRATCH 2
#endif

draw_all scratch[SCRATCHES];

unsigned int wall_color = packColor(0.5,0,0);

void display_static(void *ptr, void *) {
//	spaceinfo *s = (spaceinfo *)data;
	cpShape *shape = (cpShape*)ptr;
	
	switch(shape->collision_type) {
		case C_WALL: {
			cpPolyShape *poly = (cpPolyShape *)shape;
			scratch[0].q.push4(poly->verts, true, wall_color);
		} break;
	}
}

void object_lifetime::draw(spaceinfo *s) {
	live_connector &connect = watch->spaces[s->sid];
    cpBody *body = connect.body;
	
    switch(watch->collision_type) {
			// Do whatever here
			
		case C_PLAYER: { // Assumes  we did in fact create that body with 4 vertices...
			cpPolyShape *poly = (cpPolyShape *)connect.shapes[0];
			player_info *i = (player_info *)body->data;
			unsigned int color = i->color;
			cpVect verts[4];
			int t = s->time();
			if (occur && !i->drawing_player() && t-occur < SNAP_FADEIN_FRAMES) { // Using nonzero occur as a proxy for "snapped in". Dubious.
				double progress = double(t-occur)/SNAP_FADEIN_FRAMES;
				unsigned char *against = (unsigned char *)&color; // This is such a C programmer thing to do and I need to stop doing this sort of thing.
				for(int c = 0; c < 4; c++) {
					int dist = 0xFF-against[c];
					against[c] = 0xFF-dist*progress;
				}
			}
			for(int c = 0; c < 4; c++)
				verts[c] = cpvadd(body->p, poly->verts[c]);
//				verts[c] = cpvadd(body->p, cpvrotate(poly->verts[c], body->rot));
			
			scratch[0].q.push4(verts, true, color);
#if GLOWY_PLAYER
			if (i == player_info::current_drawing) {
				cpVect verts2[4];
				for(int c = 0; c < 4; c++) {
					verts2[c] = cpvadd(body->p, cpvmult(poly->verts[c],2));
				}
#define GLOW_OVER	((4*FPS)/3)
#define GLOW_PERIOD (GLOW_OVER*2)
				const int glow_off[4] = {0, GLOW_OVER/2, GLOW_OVER, 3*GLOW_OVER/2};
				for(int c = 0; c < 4; c++) {
#if 1
					int t = !game_halted_at ? ticks : game_halted_at;
					int where = (t + glow_off[c])%GLOW_PERIOD;
					if (where > GLOW_OVER) continue;
					double a = (where - GLOW_OVER/2)/double(GLOW_OVER/2); a = 1-fabs(a);
					unsigned int gcolor = color; unsigned char *ngc = (unsigned char *)&gcolor;
					ngc[0] *= a; ngc[1] *= a; ngc[2] *= a; ngc[3] *= a;
#else
					unsigned int gcolor = color; if (1 != c) continue;
#endif
					int g = c < 3 ? c : 1;
					scratch[0].t.push4(verts2, true, gcolor, box_glow[g]);				
				}
#if TEXTURED_PLAYER
				scratch[1].t.push4(verts, true, 0xFFFFFFFF, p2_s);				
#endif				
			}
#endif
#if VELOCITY_MAX
			cpVect v = body->v;//cpvmult(cpvforangle(2*M_PI/64*ticks), 5);
			scratch[V_SCRATCH].push4(verts, true, packColor(VELOCITY(v.x), VELOCITY(v.y), 0, 1));
#endif
		} break;
		case C_BULLET: {
			bullet_info *b = (bullet_info *)body->data;
			cpVect verts[4] = SQUARE(BULLET_RAD);
			for(int c = 0; c < 4; c++)
				verts[c] = cpvadd(body->p, verts[c]);
			scratch[0].q.push4(verts, true, b->owner->color);
			
#if VELOCITY_MAX
			cpVect v = body->v;//cpvmult(cpvforangle(2*M_PI/64*ticks), 5);
			_scratch[V_SCRATCH].push4(verts, true, packColor(VELOCITY(v.x), VELOCITY(v.y), 0, 1));
#endif
		} break; 
			
		case C_HEALTH: {
			cpPolyShape *poly = (cpPolyShape *)connect.shapes[0];
			cpVect verts[4];
			for(int c = 0; c < 4; c++)
				verts[c] = cpvadd(body->p, poly->verts[c]);
			
			unsigned int color = 0xFFFFFFFF;
			int t = s->time();
			if (!soft_launch && t - occur < SNAP_FADEIN_FRAMES) {
				double progress = double(t - occur) / SNAP_FADEIN_FRAMES; progress = 1-progress;
				color = packColor(1,progress,progress);
			}			
			
			scratch[1].t.push4(verts, true, color, box_s[1]);
		} break;
    }
}

void kash_animate::draw(spaceinfo *s) {
	double progress = 1 - (frame(s) / double(upto));
	double size_mod = 1.5 + progress;
	cpVect verts[4] = SQUARE(BULLET_RAD);
	for(int c = 0; c < 4; c++)
		verts[c] = cpvadd(p, cpvmult(verts[c], size_mod));
	scratch[1].q.push4(verts, true, packColor(progress,progress,0,progress)); //Premult
}

void snap_animate::draw(spaceinfo *s) {
	int f = frame(s);
	double progress = 1 - (f / double(upto));
	cpVect verts[] = SQUARE(PLAYER_RAD);

	unsigned int follow_frame = ::min<int>(f, follow->p.size()-1); // follow->p size will never be 0?
	cpVect p = follow->p[ follow_frame ];
	for(int c = 0; c < 4; c++)
		verts[c] = cpvadd(p, verts[c]);
	scratch[0].q.push4(verts, true, packColor(progress,progress,progress,progress));
}

void display_arrays(int w, bool matrix = false) {
	if (!scratch[w].li.empty()) {
		// display_object earlier will have filled out _scratch with a vertex array; we now just draw it
		States(false, true, matrix);
		int stride = VERT_STRIDE+COLOR_STRIDE;
		glLineWidth(4);
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch[w].li[0]);
		jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &scratch[w].li[0]+VERT_STRIDE);
		glDrawArrays(GL_LINES, 0, scratch[w].li.size()/SIZEMOD_LI);  
	}		
	
	if (!scratch[w].t.empty()) {
		// display_object earlier will have filled out _scratch with a vertex array; we now just draw it
		if (!scratch[w].tcolor) {
			States(true, false, matrix);
			jcColor4f(1,1,1,1);
			int pushed = scratch[w].t.size()/SIZEMOD_T * 6; // WHY MULTIPLY BY 6 ARG
			scratch[w].t.megaIndexEnsure(pushed);
			int stride = VERT_STRIDE+TEX_STRIDE;
			glBindTexture(GL_TEXTURE_2D, scratch[w].texture);
			jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch[w].t[0]);
			jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &scratch[w].t[0]+VERT_STRIDE);
			glDrawElements(GL_TRIANGLES, pushed, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);        		
		} else {
			States(true, true, matrix);
			int pushed = scratch[w].t.size()/SIZEMOD_TC * 6; // WHY MULTIPLY BY 6 ARG
			scratch[w].t.megaIndexEnsure(pushed);
			int stride = VERT_STRIDE+TEX_STRIDE+COLOR_STRIDE;
			glBindTexture(GL_TEXTURE_2D, scratch[w].texture);
			jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch[w].t[0]);
			jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &scratch[w].t[0]+VERT_STRIDE);
			jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &scratch[w].t[0]+VERT_STRIDE+COLOR_STRIDE);
			glDrawElements(GL_TRIANGLES, pushed, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);        					
		}
	}
	
	if (!scratch[w].q.empty()) {
		// display_object earlier will have filled out _scratch with a vertex array; we now just draw it
		States(false, true, matrix);
		int pushed = scratch[w].q.size()/SIZEMOD_Q;
		scratch[w].q.megaIndexEnsure(pushed);
		int stride = VERT_STRIDE+COLOR_STRIDE;
		jcVertexPointer(2, GL_FLOAT, stride*sizeof(float), &scratch[w].q[0]);
		jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &scratch[w].q[0]+VERT_STRIDE);
		glDrawElements(GL_TRIANGLES, pushed, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);        		
	}
}

void reset_arrays() {
	for(int c = 0; c < SCRATCHES; c++)
		scratch[c].clear();
}

void player_handler::apply_camera() {
//	States(false,true,true);
	jcLoadIdentity();
	jcScalef(camera_zoom / (aspect * arena.rad.x / arena.rad.y),camera_zoom,1);
	jcTranslatef(-camera_off.x, -camera_off.y,0);
}

void draw_all_arrays(player_handler *p) {
	display_arrays(0, true);
	display_arrays(1, true);
	
	// This is all too by-hand. But I don't know if I care at this point.
	if (p->rtrigger_x || p->rtrigger_y) {
		cpVect loopsize = li->camera_limit.size();
		if (p->rtrigger_x) {
			jcPushMatrix();
			jcTranslatef(loopsize.x*p->rtrigger_x, 0, 0);
			display_arrays(0, true);
			display_arrays(1, true);
			jcPopMatrix();
		}
		if (p->rtrigger_y) {
			jcPushMatrix();
			jcTranslatef(0, loopsize.y*p->rtrigger_y, 0);
			display_arrays(0, true);
			display_arrays(1, true);
			jcPopMatrix();
		}	
		if (p->rtrigger_x && p->rtrigger_y) {
			jcPushMatrix();
			jcTranslatef(loopsize.x*p->rtrigger_x, loopsize.y*p->rtrigger_y, 0);
			display_arrays(0, true);
			display_arrays(1, true);
			jcPopMatrix();			
		}
	}
}

// This is called when it is time to draw a new frame.
void display(void)
{	
    EnableClientState(GLCS_VERTEX); // Only relevant to GL1-- maybe shouldn't be here at all

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	goOrtho();
    jcLoadIdentity();
        
	glDisable(GL_DEPTH_TEST);
	
    // Print each space in the stack
	cpVect arena_center = cpvzero;
	
	//	for(int c = spaces.size()-1; c >= 0; c--) { // FIXME: Forward or backward?
	for(int p = 0; p < player_handler::all_players.size(); p++) {
		reset_arrays();
		
		scratch[0].texture = glow_atlas->texture; // Better place to do this?
		scratch[0].tcolor = true;
		scratch[1].texture = interface_atlas->texture; // Better place to do this?
		scratch[1].tcolor = true;
		
		player_info::current_drawing = player_handler::all_players[p]->player_initially;
		if (player_handler::all_players[p]->sin >= spaces.size()) continue; // Should never trigger
		spaceinfo *s = &(spaces[player_handler::all_players[p]->sin]);
//		ERR("P %d S %d %s", p, player_handler::all_players[p]->sin, p ? "\n":"");
		if (0) { // Ugly!!
			int t = (s->time() + 1) % now.period;
			BANNER(1) << s->sid << ": " << (t/FPS) << "." << (t%FPS);
		}
		
		cpSpaceHashEach(s->space->staticShapes, &display_static, s);
		
		for(lifetime_iter present = s->present.begin(); present != s->present.end(); present++)
			present->second->draw(s);
		
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
	
#if TEXTUREPROCESS
		bool doprocess = PROCESSSAFE && di->process;
		if (doprocess && rtt_slice) {
			jcBindFramebuffer(GL_FRAMEBUFFER_EXT, rtt_fb[0]);
			
			glViewport(0,0,(arenaw)*di->ppower,arenah*di->ppower);
			glClearColor(0,0,0,1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DNCOMMIT
			glClearColor(0.15,0.15,0.15,1);			

			player_handler::all_players[p]->apply_camera();
			
			draw_all_arrays(player_handler::all_players[p]);
#if DOUBLEPROCESS
			// Now render "velocity" layer
			rinseGl();
			jcBindFramebuffer(GL_FRAMEBUFFER_EXT, rtt_fb[1]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DNCOMMIT
			
			display_arrays(V_SCRATCH);
#endif

			jcBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
			glViewport(0,0,surfacew,surfaceh);
#define SNAP_EFFECTLASTS SNAP_FADEIN_FRAMES
			if (ticks - player_handler::all_players[p]->last_snap >= SNAP_EFFECTLASTS) {
				debug_floats[0] = 0;
			} else {
				debug_floats[0] = (1 - double(ticks - player_handler::all_players[p]->last_snap) / SNAP_EFFECTLASTS) / arenah * rtt_slice[0]->coords[2] * 100;
			}
//			arena_center = cpv((1/aspect/2 - ARENA_INSET/4 - HEALTHBAR_RAD) * (p?1:-1), 0); // Overwrite twice feels clumsy
			splut_texture(rtt_slice[0], true, p ? arena : arena.reflect_x());
		}	
		else
#endif
		{
			draw_all_arrays(player_handler::all_players[p]);
		}
	}
	
	// HUD
	if (player_handler::all_players.size()) {
		reset_arrays();
		cpVect start = {-1/aspect, 1};
		start = cpvadd(start, cpv(ARENA_INSET, -ARENA_INSET-TIMEBAR_HEIGHT/2));
		for(int p = 0; p < player_handler::all_players.size(); p++) { // Why do this twice, anyway
			// Time bar
			cpVect end = start; end.x = -end.x;
			scratch[0].li.push(start,end,true,0.5,0.5,0.5);
			
			spaceinfo *s = &(spaces[player_handler::all_players[p]->sin]);
			end = cpvadd(start, cpv(-start.x*2 * double(s->time())/now.period,0));
			{
				cpVect verts[4] = SQUARE(BULLET_RAD);
				for(int c = 0; c < 4; c++)
					verts[c] = cpvadd(verts[c],end);
				scratch[0].q.push4(verts, true, player_handler::all_players[p]->player->color);
			}
			
			// Mark drop on time bar
#define DROP_EFFECTLASTS (SNAP_FADEIN_FRAMES*2)
			if (now.lastdrop >= 0) {
				int sincedrop = now.frame - now.lastdropat;
				if (sincedrop < DROP_EFFECTLASTS) {
					double a = 1-sincedrop/double(DROP_EFFECTLASTS);
					unsigned int color = packColor(a,a,a,a);
					cpVect verts[4] = SQUARE(BULLET_RAD);
					cpVect off = cpvadd(start, cpv(-start.x*2 * double(now.lastdrop)/now.period,0));
					for(int c = 0; c < 4; c++)
						verts[c] = cpvadd(verts[c],off);
					scratch[0].q.push4(verts, true, color);					
				}
			}
						 
			start = cpvadd(start,cpv(0,-TIMEBAR_HEIGHT)); // Small waste
			
			// Health bar
			player_info *player = player_handler::all_players[p]->player;
			
			{
				cpRect outer = cpRect::square(HEALTHBAR_RAD).align_d( arena.d() ).align_l( arena.r() + HEALTHBAR_RAD ).set_u( arena.u() );
				cpRect high = outer.inset_fixed((HEALTHBAR_RAD-HEALTH_RAD)/2);
				
				cpRect low;
				int healthvalue = player->health()->value;
				
				if (healthvalue) {
					cpFloat topUsed = high.d() + high.size().y * double(healthvalue) / li->maxhealth;
					low = high.set_u(topUsed);
					high = high.set_d(topUsed);
				}

				if (!p) {
					outer = outer.reflect_x(); high = high.reflect_x(); 
					if (healthvalue) low = low.reflect_x();
				}
					
				scratch[0].q.push4(outer, true, wall_color);
				scratch[0].q.push4(high, true, packColor(0,0,0));
				if (healthvalue) { scratch[0].q.push4(low, true, player->color); }
			}

			// Somebody died. Draw a big red X
			if (player->mask & show_dead) {
				cpRect thisArena = p ? arena : arena.reflect_x(); // DRY
				scratch[0].li.push(thisArena.ul(),thisArena.dr(),true,1,0,0);
				scratch[0].li.push(thisArena.dl(),thisArena.ur(),true,1,0,0);
			}
		}
		display_arrays(0);
	}
		
	// Debug text line.. print to this with something like BANNER(FPS) << "Message"
    if (!displaying.empty()) {
        jcColor4f(1,1,1,1);
        drawText(displaying.c_str(), -1/aspect + 0.1, 0.9, 0, false, false);	
        if (displaying_life) { displaying_life--; if (!displaying_life) displaying = ""; }
        
    }    
	
	for(auto_iter i = automata.begin(); i != automata.end(); i++) {
		if ((*i)->draws)
			(*i)->draw();
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

// Apple ii screens are 40 x 24 chars; each char is 7x8 pixels.

#define TAFX (7.0/96.0 * col40hp)
#define TAFY (2.0/24  * col40hp)
#define TAU (0+TAFY*12)
#define TAL (0-TAFX*20)
#define TTU (0)
#define TTL (0)
#define TTFX (7.0/128)
#define TTFY (1.0/16)

inline void clipush(draw_all &s, int x, int y, unsigned char c, unsigned int color=0xFFFFFFFF) {
	cpFloat l = TAL+TAFX*x, u = TAU-TAFY*y;
	unsigned int tx = c%16; unsigned int ty = c/16;
	subtexture_slice tex; 
	tex.coords[0] = TTL + TTFY*tx; tex.coords[1] = TTU + TTFY*ty; // Note: TTFY in "wrong" place on purpose
	tex.coords[2] = tex.coords[0] + TTFX; tex.coords[3] = tex.coords[1] + TTFY;
	s.t.push(l,u,l+TAFX,u-TAFY, true, color, &tex);
//	ERR("LEN %d\n", s.t.size());
}

void type_automaton::draw() {
	scratch[0].clear();
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 40; x++) {
			if (screen[y][x] != ' ') {
				clipush(scratch[0],x,y,screen[y][x],scolor[y][x]);
			}
		}
	}
	scratch[0].texture = col40->texture;
	scratch[0].tcolor = true;
	display_arrays(0);
}

void freetype_automaton::draw() {
	type_automaton::draw();
	if (cursor) {
		scratch[0].clear(); // A little wasteful but WHO CARES
		clipush(scratch[0],x,y,127|(autoinv?0x80:0));
		scratch[0].tcolor = true;
		display_arrays(0);
	}
}

// ------------ AUDIO ---------------

#include "sound.h"

// Audio state:

#define NO_AUDIO 0
//#define LOG_AUDIO SELF_EDIT

FILE *audiolog = NULL; // If you find yourself needing to debug your audio, fopen() this somewhere and all audio will start getting copied to this file as 16-bit pcm
int playing = 0; // So sounding can "pause"
int tapping = 0;

#define DIGITALCLIP 1
#define SHOTDURATION 10000
#define CHIMEDURATION 5000

struct sounding { // I thought I was trying to use STL structures now...
    noise *from;
    sounding *next;
    int start;
    sounding(noise *_from, sounding *_next, int _start = 0) : from(_from), next(_next), start(_start) {}
    ~sounding() { delete from; }
};
sounding *soundroot;

pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
vector<bgaudio *> bgwait, bgwait_back, bgaudio_loose;
void bgaudio::insert() {
	pthread_mutex_lock(&audio_mutex);
	(back ? bgwait_back : bgwait).push_back(this);
	pthread_mutex_unlock(&audio_mutex);
}
void bgaudio::stop() { if (s) { s->ticks = 0; s = NULL; } } // Hacky?
void bgaudio_reset() {
	for(bgaudio_iter i = bgaudio_loose.begin(); i != bgaudio_loose.end(); i++)
		(*i)->stop();
	bgaudio_loose.clear();
}

void am_pskew(int mask, int w) {
	slidetone *tone = new slidetone(mask, SHOTDURATION, w, 0, 
									4, 1, 0.1);
	bgaudio *b = new bgaudio(tone,false,true); b->insert();
}

noise *get_kash(int mask) {
	return new notone(mask, CHIMEDURATION, (double)random()/RANDOM_MAX * 30 + 1, 0.1);
}
void am_kash(int mask) {
	noise *tone = get_kash(mask);
	bgaudio *b = new bgaudio(tone,false,true); b->insert();
}
void am_drop(int mask) {
	slidetone *tone = new slidetone(mask, SHOTDURATION, 320, 0, 
									1, 1, 0.1);
	bgaudio *b = new bgaudio(tone,false,true); b->insert();	
}

void am_drop2(int mask) {
	notone *tone = new notone(mask, CHIMEDURATION/2, 50, 0.1);
	bgaudio *b = new bgaudio(tone,false,true); b->insert();	
}

void am_pickup(int mask) {
	slidetone *tone = new slidetone(mask, SHOTDURATION, 40, 0, 
									0.05*44100, -20, 0.1); // Never actually slides!
	bgaudio *b = new bgaudio(tone,false,true); b->insert();
}

// Audio code:

#include "stb_vorbis.h"
#include "bfiles.h"

// Use at your own risk
double * audio_load(const char *name, int *outlength = NULL, bool ogg = false, double amp = 1.0) {
    char filename[FILENAMESIZE]; internalPath(filename, name);
	long length = 0; 
	short *result_raw = NULL;
	if (!ogg) {
		FILE *file = fopen(filename, "r"); // REALLY SHOULD DO SOMETHING IF THIS RETURNS NULL
		fseek( file, 0, SEEK_END ); length = ftell( file );
		fseek( file, 0, SEEK_SET );
		length /= 2; // 16 bit pcm, 2 bytes per sample
		result_raw = (short *)malloc(length*sizeof(short)); // Alloc mix makes me uncomfortable!
		fread(result_raw, sizeof(short), length, file);
		fclose(file);
	} else {
		int channels; // I know what the channel counts of my own files are
		length = stb_vorbis_decode_filename(filename, &channels, &result_raw);
	}
    double *result = new double[length];
    for(int c = 0; c < length; c++)
        result[c] = double(result_raw[c])/SHRT_MAX * amp;
    free(result_raw);
    if (outlength)
        *outlength = length;
    return result;
}

struct audio_sample {
	double *sample;
	int len;
	audio_sample(double *_sample = NULL, int _len = 0) : sample(_sample), len(_len) {}
};
vector<audio_sample> snaps_original;
// Note: 0 currently crashes!!
#define UNDERWATER_SNAPS 1
vector<audio_sample> snaps_underwater;
vector<audio_sample> snaps_underwater_rev;
// If 0, load from cache.
#define CUTUP_FIRSTRUN 0

void sample_populate(const char *name, bool ogg, const char *cutname, double hthresh, double lthresh, int badspan, vector<audio_sample> &into) {
	int len = 0;
	double *sample = audio_load(name, &len, ogg);
	if (!len) {
		ERR("NO LOAD");
	} else {
#if CUTUP_FIRSTRUN
		bool on = false;
		int start = 0;
		for(int c = 0; c < len; c++) {
			if (!on) {
				if (sample[c] > hthresh) {
					on = true;
					start = c;
				}
			}
			if (on) {
				if (c-start >= badspan) {
					double foundmax = 0;
					for(int d = c-badspan; d < c; d++) {
						double fs = fabs(sample[d]);
						if (fs > foundmax)
							foundmax = fs;
					}
					if (foundmax < lthresh || c+1>=len) {
						on = false;
						ERR("%d: %d to %d\n", (int)into.size(), start, c);
						into.push_back( audio_sample( &sample[start], c-start ) );
					}
				}
			}
		}
		ERR("ON? %s\n", on?"Y":"N");
		
		ofstream f;
		f.open(cutname, ios_base::out | ios_base::binary | ios_base::trunc);
		if (!f.fail()) {
			int temp = into.size();
			BWRITE(temp);
			for(int c = 0; c < into.size(); c++) {
				temp = into[c].sample - sample;
				BWRITE(temp); BWRITE(into[c].len);
			}
		}			
#else
		char cfname[FILENAMESIZE];
		internalPath(cfname, cutname);		
		ifstream f;
//		f.exceptions( ios::eofbit | ios::failbit | ios::badbit ); // ... do we care?
		f.open(cfname, ios_base::in | ios_base::binary);
		int isize; BREAD(isize);
		for(int c = 0; c < isize; c++) {
			int start, len;
			BREAD(start); BREAD(len);
			into.push_back(audio_sample(&sample[start], len));
		}
#endif
	}
}

void am_snap(int mask, int sin, int new_sin) {
	if (!snaps_original.size())
		return;
		
#if 0
	static int guh = 0;
	int which = guh++ % snaps_original.size();
	ERR("GUH %d\n", which);
#else
	int which = random() % snaps_underwater.size();
#endif
	bgaudio *b = new bgaudio( new sampletone(mask, snaps_original[which].sample, snaps_original[which].len ), false, true);
	b->insert();
	
	if (sin < 0 || new_sin < 0) return;
	
	int from = spaces[sin].time();
	int to = spaces[new_sin].time();
	
#if UNDERWATER_SNAPS 
	// Note: Events are not dependent on anything.
	noise *samp2 = new sampletone(0, snaps_underwater_rev[which].sample, snaps_underwater_rev[which].len );
	
	int surwl = snaps_underwater[which].len;
	
	int rt = (to - int(floor(surwl / 44100.0 * FPS / 2)) + now.period) % now.period;
	audio_event *snap_event = new audio_event(rt, samp2);
	now.moments[rt].events.push_back( snap_event );
	
	int outmask = spaces[new_sin].visible & ~mask;
	if (outmask) {
		b = new bgaudio( samp2->clone(outmask), false, true);
		b->insert();
	}
	
	noise *samp3 = new sampletone(0, snaps_underwater[which].sample, surwl );

	int rf = from;
	audio_event *snap_event2 = new audio_event(rf, samp3);
	now.moments[rf].events.push_back( snap_event2 );

	int inmask = spaces[sin].visible & ~mask;
	if (inmask) {
		b = new bgaudio( samp3->clone(inmask), false, true);
		b->insert();
	}
#endif
	
}

void audio_init() {
#if LOG_AUDIO
	audiolog = fopen("/tmp/OUTAUDIO", "w");
#endif    
	
#if 1
	sample_populate("snaps_original.ogg", true, "snaps_original_cuts.obj", 0.000500, 0.000500, 1024, snaps_original);
#if UNDERWATER_SNAPS
	sample_populate("snaps_underwater.ogg", true, "snaps_underwater_cuts.obj", 0.100000, 0.050000, 1024, snaps_underwater);
	for(int c = 0; c < snaps_underwater.size(); c++) {
		int len = snaps_underwater[c].len;
		double *s = snaps_underwater[c].sample;
		double *n = new double[len];
		for(int d = 0; d < len; d++) {
			double mod = 0.5;
			if (d < 10)
				mod *= (d/10.0);
			if (d > len/2)
				mod *= ( (len-d-1) / (len/2.0) );
			s[d] *= mod;
			n[len-d-1] = s[d];
		}
		snaps_underwater_rev.push_back(audio_sample(n, len));
	}
#endif
	ERR("SNAPS? %d vs %d\n", (int)snaps_original.size(), (int)snaps_underwater.size());
	
#endif
}

void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);
	
	pthread_mutex_lock(&audio_mutex);	
	if (!bgwait.empty()) { // "Pull items out of STL container and push to beginning of linked list"
		for(bgaudio_iter i = bgwait.begin(); i != bgwait.end(); i++) {
			soundroot = new sounding( (*i)->s, soundroot, playing );
			if ((*i)->discard_wrapper) delete *i;
		}
		bgwait.clear();
	}
	if (!bgwait_back.empty()) { // "Pull items out of STL container and push to end of linked list"
		sounding **tail; for(tail = &soundroot; *tail; tail = &(*tail)->next);
		for(bgaudio_iter i = bgwait_back.begin(); i != bgwait_back.end(); i++) {
			*tail = new sounding( (*i)->s, *tail, playing );
			tail = &(*tail)->next;
			if ((*i)->discard_wrapper) delete *i;
		}
		bgwait_back.clear();
	}
	pthread_mutex_unlock(&audio_mutex);	
	
	for(sounding **n = &soundroot; *n; n = &(*n)->next) {
		(*n)->from->boundary(slen/2); // Divide on channels
		//		ERR("%d: %x\n", x++, *n);
	}	
	
	for(int c = 0; c < slen; c+=2) { // Write your audio code in here. My default just fires a simple list of "sounding" objects:
		double value = 0, valuer = 0;
                        
        for(sounding **n = &soundroot; *n;) {
            sounding *now = *n;
            
            if (playing < (*n)->start) {
                n = &(*n)->next; // Slightly duplicative of code
                continue;
            }
            
            now->from->to(value, valuer);
            
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
        if (valuer < -1) valuer = -1;
        if (valuer > 1) valuer = 1; // Maybe add like a nice gate function... I dunno
#endif
        
		samples[c] = value*SHRT_MAX;
		samples[c+1] = valuer*SHRT_MAX;
        tapping++;
	}
	if (audiolog) { // For debugging audio
		fwrite(stream, sizeof(short), slen, audiolog);
	}
}
