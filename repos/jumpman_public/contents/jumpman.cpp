// BUSINESS LOGIC

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

#ifdef WINDOWS
inline long htonl (long i) { // Mingw has failed me but windows only ever has one endianness anyway.
	long r;
    char *p = (char *)&r;
	char *c = (char *)&i;
	    p[0] = c[3];
        p[1] = c[2];
        p[2] = c[1];
        p[3] = c[0];
	return *((long *)p);
}
#endif
#ifdef LINUX
#include <netinet/in.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "chipmunk.h"
#include "lodepng.h"
#include "sound.h"
#include "internalfile.h"
#include "tinyxml.h"
#include "color.h"
#include "jumpman.h"

#define SLOWMO 0
#define GRAVITY cpv(0.0f, -900.0f)
//#define GRAVITY cpvzero
#define ALLOW_SPINNING 0
#define TILT_DEBUG 0

bool special_ending_floor_construction = false;

inline double round_to(double a, double b) { a /= b; a += 0.5; a = floor(a); a *= b; return a; }

inline void rotl(unsigned char &i) {
	i = (i << 1) | (i & 0x80 ? 1 : 0);
}

inline void rotr(unsigned char &i) {
	i = (i >> 1) | (i & 0x01 ? 0x80 : 0);
}

/* Number of terrain points. */
int terrain_points = 200;

/* The rigid bodies for the chassis and wheel. They are declared here
   so they can be accessed from the update function. */
cpBody *chassis = NULL;
cpShape *chassisShape = NULL;

hash_map<unsigned int, plateinfo *> pinfo;
hash_map<string, pair<scoreinfo, scoreinfo> > scores;

int jumpman_flag = 0;
string currentScoreKey;
void flag_rollover();

bool on_land = false;
bool was_on_land = false;
bool jumping = false;

int currentJumpmanFrame();
plateinfo *plate_construct(const char *fmt, int count, float, float, float, platebehave);
void construct_mountain();
int started_moving_at_ticks = 0;

int wantrot = 0;
int rotstep = 0;
double roten = 0;
int desiredEnding = 0;

bool surplusSpinning = false;

unsigned int chosenTool = C_FLOOR;

bool haveLineStart = false, dragPoison = false; int lineStartX, lineStartY;

EditMode edit_mode = EPlayground; 

extern char *readableControl(int c);

TiXmlDocument *editing = NULL;
string editingPath;
TiXmlNode *editLevel = NULL;
vector<slice *> editSlice;
vector<string> editSlicePath;
int editLayer = 0;

vector<cpContact *> bounces;
vector<cpShape *> deaths;
vector<cpShape *> splodes;
vector<cpContact *> shatters;
cpShape *bombplease = NULL;
vector<cpShape *> shaking, shaking2; // Shaking2 just gets deleted.
bool fogged = false; // When true, can't rotate.
int exit_direction = 1; // Should almost always be 1
bool exiting_entire_game = false; // Always 1 except when game beaten

cpFloat input_power_modifier = 1.0; // Are we reflected?

/* This variable will be used to store the user input. When the mouse
   button is down, the power becomes 1.0. */
cpFloat input_power = 0.0;
cpFloat input_power_last_facing = 1.0;

cpFloat input_power_unpinned = 0.0;
cpFloat input_power_last_facing_unpinned = 1.0;

cpVect cpvreflect(cpVect v, cpVect axis) {
	return cpvadd( cpvneg(v), 
		cpvmult(axis, 2*cpvdot(v, axis)) );
}

void slice::construct(const char *filename, bool reallyconstruct, spaceinfo *tiltfrom) {
	LodePNG_Decoder decoder;
	LodePNG_Decoder_init(&decoder);
	unsigned char* buffer;
	unsigned char* image;
	size_t buffersize, imagesize;
	
	LodePNG_loadFile(&buffer, &buffersize, filename); /*load the image file with given filename*/

	if ( !buffer || buffersize <= 0 ){
	  REALERR("Couldn't open file: %s\n", filename);
	  FileBombBox(filename);
	}

	LodePNG_decode(&decoder, &image, &imagesize, buffer, buffersize); /*decode the png*/

	if (reallyconstruct)
		construct(decoder, image, tiltfrom);
	else
		consume(decoder, image);
		
	free(buffer); free(image); // FIXME: Mixing malloc and free in a single program... :(
}

void slice::construct(LodePNG_Decoder &decoder, unsigned char* image, spaceinfo *tiltfrom) {
	consume(decoder, image); // Must consume to construct
	border(tiltfrom);	
	construct();
}

void slice::consume(LodePNG_Decoder &decoder, unsigned char* _image) {
	int width = decoder.infoPng.width; height = decoder.infoPng.height;
		  
	init(width, height);
	uint32_t *image = (uint32_t *)_image;
	for(int x = 0; x < decoder.infoPng.width; x++) {
		for(int y = 0; y < decoder.infoPng.height; y++) {
			pixel[x][y] = htonl(image[y * decoder.infoPng.width + x]);
			if (0xFFFFFFFF == pixel[x][y])
				pixel[x][y] = 0;
		}
	}
}

// A certain consumer of PNG data needs a one-pixel buffer around the edge of the image that repeats the other edge of the image.
void slice::border(spaceinfo *tiltfrom) {
	bool repeat = tiltfrom && tiltfrom->repeat && width == height; // Note: REPEAT FEATURE REQUIRES YOU TO BE SQUARE
	
	if (tiltfrom) tiltfrom->base_width = 36*(this->width); // Note: THIS REALLY SHOULD BE HAPPENING SOMEWHERE MORE LOGICAL
	
	if (repeat) {
		int oldwidth = width;
		width+=2;
		height+=2;
		
		{ // First we have to resize our existing pixel array:
			uint32_t **newpixel;
			newpixel = new uint32_t*[width]; 
			for(int x = 0; x < width; x++) {
				newpixel[x] = new uint32_t[width]; 
				for(int y = 0; y < width; y++) {
					int x2 = x - 1; int y2 = y - 1;
					if (x2 >= 0 && y2 >= 0 && x < (width-1) && y < (width-1))
						newpixel[x][y] = pixel[x2][y2];
					else
						newpixel[x][y] = 0;					
				}
			}
			for(int x = 0; x < oldwidth; x++) delete[] pixel[x]; delete[] pixel;		
			pixel = newpixel;
		}

		uint32_t *edges[4][width]; // It seems like this whole thing could be a lot more efficient but...
		uint32_t *inges[4][width];
		
		for(int d = 0; d < width; d++) {
			edges[0][d] = &pixel[d][0];			
			edges[1][d] = &pixel[0][d];			
			edges[2][d] = &pixel[d][height-1];	
			edges[3][d] = &pixel[width-1][d];	

			inges[0][d] = &pixel[d][1];	
			inges[1][d] = &pixel[1][d];			
			inges[2][d] = &pixel[d][height-2];	
			inges[3][d] = &pixel[width-2][d];	
		}
		
		for(int r = 0; r < 2; r++) {
			for(int c = 0; c < 4; c++) {
				int tilt = tiltfrom->getReprot(c);
				int rot = tilt&REPROTROT;
				int ref = tilt&REPROTREF;
				
				bool reverse = rot/2; // Invert order?
				
				int c2 = 10 - c;
				c2 += rot;
				
				if (c%2) {
					c2 += 2;
				}
				
				if (rot%2) {		// Why the hell do I have to do it this way? I have no idea!
					if (!(c%2)) {
						reverse = !reverse;
					}
				}
				
				if (ref) {
					if (rot%2 && c>=2) {
						c2 += 2;
						reverse = !reverse;
					}
						
					if (c%2) {
						c2 += 2;
					} else {
						reverse = !reverse;
					}
				}
				
				c2 %= 4;
				
				for(int d = 0; d < width; d++) {
					int d2 = d;
					if (reverse) d2 = width-d2-1;
					if (C_WRAPTYPE(*inges[c2][d2])) {
						*edges[c][d] = *inges[c2][d2];
					}
				}
			}
		}
	}
}

void slice::construct() {
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			unsigned int color = pixel[x][y];
			bool present = C_FLOORTYPE(color);
//			ERR("@ [%d, %d]: %s\n", x, y, present?"Y":"N");
			if (special_ending_floor_construction)
				present = (color && color != C_FLOOR);
				
			if (present) {
				int bh, bw;
				
				for(bw = x; bw < width && color == pixel[bw][y]; bw++);
				bw -= x;
				
				bh = 0;
				bool rectangular = true;
				while (rectangular && y+bh < height) {
					for(int cx = 0; cx < bw; cx++) {
						if (pixel[x + cx][y + bh] != color) {
							rectangular = false;
							break;
						} 
					}
					if (rectangular)
						bh++;
				}
				
				//ERR("BLOCK AT [%d,%d] dimensions [%d,%d] color %u\n", x, y, bw, bh, color);
				for(int cx = x; cx < x+bw; cx++) {
					for(int cy = y; cy < y+bh; cy++) {
						pixel[cx][cy] = 0;
					}
				}
				
				blocks.push_back( block::b(x, y, bw, bh, color ) );
			}
		}
	}
}

slice * slice::clone() {
	slice *s = new slice();
	s->init(width, height);
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			s->pixel[x][y] = pixel[x][y];
		}
	}
	return s;
}

inline cpVect rot(const cpVect &in, camera_type level = level[jumpman_s].camera) {
	if (cam_fixed == level)
		return in; // OH GOD THIS IS A TERRIBLE KLUDGE
	else
		return cpvrotate(in, cpvforangle(roten));
}

double frot(const double &in) {
	double o = fmod(in+roten, 2*M_PI);
	if (o < -M_PI) o += 2*M_PI;
	if (o > M_PI) o -= 2*M_PI;
	return o;
}

double drot(const double &in) { // "Don't Rotate"
	double o = fmod(in, 2*M_PI);
	if (o < -M_PI) o += 2*M_PI;
	if (o > M_PI) o -= 2*M_PI;
	return o;
}

void rotatePoints(float &x, float &y, const cpVect &by) {
	cpVect p = cpvrotate(cpv(x, y), by);
	x = p.x;
	y = p.y;
}

struct AdHocRotObjectData { // I DON'T FEEL VERY COMFORTABLE WITH THIS
	spaceinfo *space;
	double by;
};

void rotObject(void *ptr, void *_data)
{
	cpShape *shape = (cpShape*)ptr;
	AdHocRotObjectData *data = (AdHocRotObjectData *)_data;
	//ERR("Shape %x type %x data %x ok? %s\n", shape, shape->collision_type, shape->data, okRot(shape->data)?"Y":"N");
	double by = data->by;
	bool tiltref = shape->body->data && ((enemy_info *)shape->body->data)->tiltref;
	if (tiltref && cam_fixed != data->space->camera)
		by = -by;

	cpVect off = cpvforangle(by);
		
	if (okRot(shape->body->data)) {	
		if (cam_fixed == data->space->camera) {
			// FIXME: Is this what we want?
			shape->body->p = cpvrotate( shape->body->p, off );
			shape->body->v = cpvrotate( shape->body->v, off );
			if (tiltref) {
				cpBodySetAngle(shape->body, shape->body->a + 2*by);
				((enemy_info *)shape->body->data)->tiltg = cpvrotate(((enemy_info *)shape->body->data)->tiltg, cpvforangle(2*by));				
			}
		} else {
			cpBodySetAngle(shape->body, shape->body->a + by);
			((enemy_info *)shape->body->data)->tiltg = cpvrotate(((enemy_info *)shape->body->data)->tiltg, off);
		}
	} else { // noRot objects rotate as if they were static
		if (cam_fixed == data->space->camera) { // WRONGNESS
			
			if (tiltref) {
				cpBodySetAngle(shape->body, shape->body->a - by);
				((enemy_info *)shape->body->data)->tiltg = cpvrotate(((enemy_info *)shape->body->data)->tiltg, off);				
			} else
				cpBodySetAngle(shape->body, shape->body->a + by );
			shape->body->p = cpvrotate( shape->body->p, off );
			shape->body->v = cpvrotate( shape->body->v, off );
		} else {
			((enemy_info *)shape->body->data)->tiltg = cpvrotate(((enemy_info *)shape->body->data)->tiltg, off);
		}
	}
}

void rotStatic(void *ptr, void *_data) // OCTAGON ONLY
{
	cpShape *shape = (cpShape*)ptr;
	AdHocRotObjectData *data = (AdHocRotObjectData *)_data;

	if (C_FLOORTYPE(shape->collision_type) && shape->collision_type != C_LOOSE) return;

	cpVect off = cpvforangle(data->by);
	shape->body->p = cpvrotate( shape->body->p, off );
	cpBodySetAngle(shape->body, shape->body->a + data->by);
}

void rotenDelta(double delta) {
	haveLineStart = false;
	dragPoison = true;
	if (jumpman_s >= level.size())
		return;

	if (0 == rotstep) {
		surplusSpinning = (((enemy_info *)chassis->data)->tiltref) ^ (input_power_modifier<0);
#if TILT_DEBUG
		ERR("ROTSTEP %d surplusSpinning %s ctiltref %s input_mod %s, reflect %s\n", rotstep, surplusSpinning?"Y":"N", ((enemy_info *)chassis->data)->tiltref?"Y":"N", input_power_modifier<0?"Y":"N", (tiltrightnow()%REPROTREF)?"Y":"N");
#endif
	}
	if (surplusSpinning) {
		surplusSpin += 2*delta;
	}

	if (cam_fixed == level[jumpman_s].camera)
		delta *= -1;

	roten += delta;
	roten = fmod(roten, 2*M_PI);

	if (cam_fixed == level[jumpman_s].camera) {
		cpBodySetAngle(level[jumpman_s].staticBody, level[jumpman_s].staticBody->a + delta);
	} else {
		level[jumpman_s].space->gravity = rot(GRAVITY);
	
		cpVect off = cpvforangle(delta);
		level[jumpman_s].master_tiltg = cpvrotate(level[jumpman_s].master_tiltg, off);
	}
	
	{
		AdHocRotObjectData rotData = {&level[jumpman_s], delta};
		cpSpaceHashEach(level[jumpman_s].space->activeShapes, &rotObject, &rotData);

		if (cam_fixed == level[jumpman_s].camera) {
			cpSpaceHashEach(level[jumpman_s].space->staticShapes, &rotStatic, &rotData);
			
			cpVect off = cpvforangle(delta);
			rotatePoints(jumpman_x, jumpman_y, off);

			cpSpaceRehashStatic(level[jumpman_s].space);
		}
	}
}

#define TILTBY_INVALID 1000

inline void untilt(spaceinfo *s, cpShape *shape, int tiltnow) {
	if (0 == tiltnow) {
		if (s->space) // This shouldn't ever fail, but
			cpSpaceRemoveBody(s->space, shape->body);
	} else if (0 < tiltnow) {
		s->tilt.erase((unsigned int)shape);
	} else if (0 > tiltnow) {
		s->mans.erase((unsigned int)shape);
	}
}
inline void retilt(spaceinfo *s, cpShape *shape, int tiltnew) {
	if (0 == tiltnew) {
		cpSpaceAddBody(s->space, shape->body);
	} else if (0 > tiltnew) {
		s->mans[(unsigned int)shape] = shape; // This should never happen
	} else {
		s->tilt[(unsigned int)shape] = shape;
		s->anytilts = true;
	}
	if (shape->body->data)
		((enemy_info *)shape->body->data)->tiltid = tiltnew; // If you don't have enemy info, don't call this

}
inline void SpaceRemoveBody(spaceinfo &s, cpShape *shape) {
	untilt(&s, shape, ((enemy_info *)shape->body->data)->tiltid);
}
inline void SpaceAddBody(spaceinfo &s, cpShape *shape) {
	retilt(&s, shape, ((enemy_info *)shape->body->data)->tiltid);
}

void externSpaceRemoveBody(spaceinfo &s, cpShape *shape) { // Yeah I dunno.
	SpaceRemoveBody(s, shape);
}

void wrapObject(void *ptr, void *_data)
{
	cpShape *shape = (cpShape*)ptr;
	spaceinfo *s = (spaceinfo *)_data;

	switch(s->repeat) {	case repeat_normal: {
		for(int xory = 0; xory < 2; xory++) { // Run through this twice, once for each axis. It turned out that trying to do both axes at once confused the tilt.
			double mag;
			int tiltby = 0;
			bool reflectx = false, reflecty = false;
			cpVect ref = s->staticBody->rot;
			
			mag = cpvdot(shape->body->p, ref);
//			ERR("%x TEST x %lf > %lf\n", shape, mag, s->repeat_every/2);
			if (xory && fabs(mag) > s->repeat_every/2) {
				cpVect offset = cpvmult(ref, s->repeat_every*(mag>0?-1:1));
				shape->body->p = cpvadd(shape->body->p, offset);

				int rr = s->reprotForDir(false, mag>0);
				int reprot = rr&REPROTROT;
				reflectx = rr&REPROTREF;

				if ((rr == 5 || rr == 7) && !(mag<0)) reprot += 2;
				//if (rr == 5) reprot += 2;

//				ERR("#2x crossover false, %s: %d = %d, %d offs [%f,%f]\n", mag*input_power_modifier>0?"true":"false", rr, reprot, reflectx, offset.x, offset.y);
				
				tiltby = reprot;
								
				if (C_JUMPMAN == shape->collision_type && s->camera == cam_track && edit_mode != EWalls) {
//					unsigned int greprot = tiltrightnow();
//					unsigned int grotate = greprot&REPROTROT;
//					unsigned int greflect = greprot&REPROTREF;

//				ERR("#2x crossover false, %s: %d = %d, %d offs [%f,%f]\n", mag*input_power_modifier>0?"true":"false", rr, reprot, reflectx, offset.x, offset.y);

				int thistilt = tiltrightnow();
				if (thistilt) {
					//offset = cpvrotate(offset, ref);
					if (thistilt&REPROTREF)
						offset.x = -offset.x;
					offset = cpvunrotate(offset, cpvforangle((thistilt&REPROTROT)*M_PI/2));
				}
					offset.y = -offset.y; // THIS WORKS AND I HAVE NO IDEA WHY. I'M TERRIFIED.

//					ERR("1rescanBEF %lf,%lf : rot %d ref %d offs [%f,%f] p [%f,%f] refref [%f,%f]\n", rescan.x/level[jumpman_s].repeat_every, rescan.y/level[jumpman_s].repeat_every, grotate, greflect, offset.x, offset.y, chassis->p.x, chassis->p.y, ref.x, ref.y);

					rescan = cpvadd(rescan, offset);

//					 greprot = tiltrightnow();
//					 grotate = reprot&REPROTROT;
//					 greflect = reprot&REPROTREF;

//					ERR("1rescanAFT %lf,%lf : rot %d ref %d p [%f,%f]\n", rescan.x/level[jumpman_s].repeat_every, rescan.y/level[jumpman_s].repeat_every, grotate, greflect, chassis->p.x, chassis->p.y);
				}
			}
			ref = cpvperp(ref);
			mag = cpvdot(shape->body->p, ref);
//			ERR("%x TEST y %lf > %lf\n\n", shape, mag, s->repeat_every/2);
			if (!xory && fabs(mag) > s->repeat_every/2) {
				cpVect offset = cpvmult(ref, s->repeat_every*(mag>0?-1:1));
				shape->body->p = cpvadd(shape->body->p, offset);
				
				int rr = s->reprotForDir(true, mag>0);
				int reprot = rr&REPROTROT;
				reflecty = rr&REPROTREF;

				if ((rr == 5 || rr == 7) && !(mag<0)) reprot += 2;

				tiltby = reprot;

				if (C_JUMPMAN == shape->collision_type && s->camera == cam_track && edit_mode != EWalls) {
//					unsigned int greprot = tiltrightnow();
//					unsigned int grotate = reprot&REPROTROT;
//					unsigned int greflect = reprot&REPROTREF;

//					ERR("#2y crossover true, %s: %d = %d, %d offs [%f,%f]\n", mag*input_power_modifier>0?"true":"false", rr, reprot, reflecty, offset.x, offset.y);
					
					int thistilt = tiltrightnow();
					if (thistilt) {
						if (thistilt&REPROTREF)
							offset.x = -offset.x;
						offset = cpvunrotate(offset, cpvforangle((thistilt&REPROTROT)*M_PI/2));
					}

					offset.x = -offset.x; // THIS WORKS AND I HAVE NO IDEA WHY. I'M TERRIFIED.
					
//					ERR("2rescanBEF %lf,%lf : rot %d ref %d offs [%f,%f] p [%f,%f] refref [%f,%f]\n", rescan.x/level[jumpman_s].repeat_every, rescan.y/level[jumpman_s].repeat_every, grotate, greflect, offset.x, offset.y, chassis->p.x, chassis->p.y, ref.x, ref.y);

					rescan = cpvadd(rescan, offset);

//					 greprot = tiltrightnow();
//					 grotate = greprot&REPROTROT;
//					 greflect = greprot&REPROTREF;

//					ERR("2rescanAFT %lf,%lf : rot %d ref %d p [%f,%f]\n", rescan.x/level[jumpman_s].repeat_every, rescan.y/level[jumpman_s].repeat_every, grotate, greflect, chassis->p.x, chassis->p.y);
				}
			}
						
			if (tiltby) {
				enemy_info *info = (enemy_info *)shape->body->data;
				double angle = tiltby*M_PI/2;
				cpVect vangle = cpvforangle(angle);
				shape->body->p = cpvrotate(shape->body->p, vangle);
				shape->body->v = cpvrotate(shape->body->v, vangle);
				
				cpBodySetAngle(shape->body, shape->body->a + angle);
				info->tiltg = cpvrotate(info->tiltg, vangle);
				
				if (info && info->tiltid == 0) {
					untilt(s, shape, 0);
					retilt(s, shape, 1);
#if TILT_DEBUG
					ERR("TILTBY %d\n", tiltby);
#endif
				}
			}
			
			if (reflectx || reflecty) { // Almost all of this was derived experimentally. I don't know why it works or, really, whether it works.
				enemy_info *info = (enemy_info *)shape->body->data;
				cpVect xaxis = shape->body->rot;
				cpVect yaxis = cpvperp(xaxis);
				
#if TILT_DEBUG
				cpFloat xbefore = cpvdot(shape->body->p, xaxis);		
				cpFloat ybefore = cpvdot(shape->body->p, yaxis);		

				ERR("OKAY ROT: %f TILT: %d\n", shape->body->a/M_PI, info->tiltid);
				
				ERR("Before [%f,%f]\n", shape->body->p.x, shape->body->p.y);
#endif	
				shape->body->p = cpvreflect(shape->body->p, cpvperp(s->staticBody->rot));
				shape->body->v = cpvreflect(shape->body->v, cpvperp(s->staticBody->rot));

#if TILT_DEBUG				
				ERR("After [%f,%f]\n", shape->body->p.x, shape->body->p.y);

				cpFloat xafter = cpvdot(shape->body->p, xaxis);
				cpFloat yafter = cpvdot(shape->body->p, yaxis);
									
				bool xflip = (xbefore < 0) ^ (xafter < 0);
				bool yflip = (ybefore < 0) ^ (yafter < 0);
		
				ERR("xbefore %f xafter %f xflip? %s\n", xbefore, xafter, xflip?"Y":"N");
				ERR("ybefore %f yafter %f yflip? %s\n", ybefore, yafter, yflip?"Y":"N");
				if (rotstep) {
					ERR ("rs %d wr %d\trx %s fx %s ry %s fy %s; ss %s _ tr %s im %s - TR %s IM %s\n", rotstep, wantrot, reflectx?"Y":"N", xflip?"Y":"N", reflecty?"Y":"N", yflip?"Y":"N", surplusSpinning?"Y":"N", ((enemy_info *)chassis->data)->tiltref?"Y":"N", input_power_modifier<0?"Y":"N", !(xflip && !yflip)?"Y":"N", !(reflectx && reflecty)?"Y":"N");
				}
#endif

				if (C_JUMPMAN == shape->collision_type && !(reflectx && reflecty)) {
					input_power_last_facing = -input_power_last_facing;
					input_power_modifier = -input_power_modifier;
					input_power = -input_power;
#if TILT_DEBUG
					if (rotstep) {
						sbell.w = 1 ? 200 : 50;
						sbell.reset();
					}
#endif
				}
				
				if (1) { // Used to this said !(xflip && !yflip). I think it was to work around another bug, which I since fixed. Why was it there, though?
					if (info && info->tiltid == 0) {
						untilt(s, shape, 0);
						retilt(s, shape, 1);
					}

#if TILT_DEBUG
					ERR("Tiltby-REFLECT [%d or\n\n", (yflip?2:0));
#endif

					if (cam_fixed != s->camera) {
						cpBodySetAngle(shape->body, -shape->body->a);
						info->tiltg = cpvforangle(-cpvtoangle(info->tiltg)); // is this different from cpvneg?
					} else {
//						ERR("\t%f ->", shape->body->a);

						cpBodySetAngle(shape->body, s->staticBody->a-(shape->body->a - s->staticBody->a));
						info->tiltg = cpvforangle(s->staticBody->a-(cpvtoangle(info->tiltg) - s->staticBody->a)); // is this different from cpvneg?
					
//						ERR("%f (%f)\n", shape->body->a, s->staticBody->a);
					}
				
					info->tiltref = !info->tiltref;
				}
			}
		} break; }
	}
}

void doFindReplace(string &src, const string &find, const string &replace) {
	int foundAt;
	while (std::string::npos != (foundAt = src.find(find))) {
		src.replace(foundAt, find.size(), replace);
	}
}

void shakeLoose(void *ptr, void *_data) // OCTAGON ONLY
{
	cpShape *shape = (cpShape*)ptr;

	if (shape->collision_type == C_LOOSE)
		shaking.push_back(shape);
	else if (shape->collision_type == C_LOOSE2 || shape->collision_type == C_LOOSE3)
		shaking2.push_back(shape);
}

int jumpman_lands(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	for(int c = 0; !on_land && c < numContacts; c++) {
		cpFloat angle = cpvtoangle(cpvunrotate(cpvmult(contacts[c].n, normal_coef), chassis->rot)); // TODO: World can rotate
		angle += M_PI/2; // rotate 90 for some reason?
		angle = drot(angle);

//		ERR("%f <= %f <= %f %s\n", M_PI/3, angle, -M_PI/3, angle <= M_PI/3 && angle >= -M_PI/3?"(LAND)":"");  
		if (angle <= M_PI/3 && angle >= -M_PI/3) { // Unless it's too steep...
			if (!was_on_land) { // LANDING FR REAL
				sland.reset();
				jumping = false;
				
				if (!level[jumpman_s].landed) {
					while (level[jumpman_s].messages.size() > 0) {
						string text = level[jumpman_s].messages.front();
						
						doFindReplace( text, "!XZLEFT", readableControl(0) );
						doFindReplace( text, "!XZRIGHT", readableControl(1) );
						doFindReplace( text, "!XZJUMP", readableControl(2) );
						doFindReplace( text, "!XZROTL", readableControl(3) );
						doFindReplace( text, "!XZROTR", readableControl(4) );
						doFindReplace( text, "!XZCONTROLS", readableControl(5) );
						doFindReplace( text, "!XZQUIT", readableControl(6) );

						addFloater(text);
						level[jumpman_s].messages.pop();
					}
					level[jumpman_s].landed = true;
				}
			}
			on_land = true; // MAYBE JUST PERSISTING
		}
	}
	return true;
}

int jumpman_dies(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	if (jumpman_unpinned || invincible()) return false;
	jumpmanstate = jumpman_wantsplode; 
	return false;
}

int jumpman_paint(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	a->layers = ~b->layers;
	{
		int num = (int)data;
		int lc = 1 << (level[num].layers/2);
		sbell.w = 100;
		sbell.w *= lc;
		sbell.w /= a->layers;
		ERR("l %d lc %d w %d\n", a->layers, lc, sbell.w); 
		sbell.reset();
	}
	return false;
}

int jumpman_exit(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	if (jumpman_unpinned || edit_mode == EWalls) return false;
	jumpmanstate = jumpman_wantexit; 
	exit_direction = 1;
	return false;
}

int jumpman_unexit(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	if (jumpman_unpinned || edit_mode == EWalls) return false;
	if (jumpman_s == 0) {
		sland.reset();
		return false;
	}
	jumpmanstate = jumpman_wantexit; 
	exit_direction = -1;
	return false;
}

int jumpman_bombs(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	bombplease = b;
	return false;
}

int jumpman_checkpoint(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	jumpman_x = chassis->p.x;
	jumpman_y = chassis->p.y;
	for(jumpman_l = 0; jumpman_l < 32; jumpman_l++) {
		if (chassisShape->layers & (1 << jumpman_l))
			break;
	}
	return false;
}

int jumpman_fog(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	fogged = true; 
	return false;
}

int shrapnel_bounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	if (shatter.ticks < 25000) {
		for(int c = 0; c < numContacts; c++) {
			shatters.push_back(&contacts[c]);
		}
	}
	return true;
}

// Kludges to get around poor interactions between chipmunk and my various bounce methods:
int sticky_hbounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data);
int swoop_bounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data);
#define PUSHBACK \
			if (num >= 0) switch (b->collision_type) { \
				case C_MARCH: \
					march_hbounce(b, a, contacts, numContacts, -normal_coef, (void *)(-1)); \
					break; \
				case C_STICKY: \
					sticky_hbounce(b, a, contacts, numContacts, -normal_coef, (void *)(-1)); \
					break; \
				case C_SWOOP: \
					swoop_bounce(b, a, contacts, numContacts, -normal_coef, (void *)(-1)); \
					break; \
				default: break; \
			}

int march_hbounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	bool bounced = false;
	bool zoomed = false;
	int num = (int)data;
	
	for(int c = 0; !(bounced && zoomed) && c < numContacts; c++) {
		cpFloat angle = 
			cpvtoangle(cpvmult(contacts[c].n, normal_coef))
			- a->body->a
			+ M_PI/2; // TODO: World can rotate
		while (angle < -M_PI) angle += 2*M_PI;
		while (angle > M_PI) angle -= 2*M_PI;
		
//		angle += 3*M_PI/4; // WTF!
//		angle = drot(angle);
//		ERR("%f %s\n", angle, !(    (angle >= M_PI/4 && angle <= 3*M_PI/4)
//			 || (angle <= -M_PI/4 && angle >= -3*M_PI/4) )?"(WALLWALLWALLWALLWALL)":"");  
		
		if (!bounced && (    (angle >= M_PI/3 && angle <= 5*M_PI/6)
			 || (angle <= -M_PI/3 && angle >= -5*M_PI/6) )) {
//		ERR("%f <= %f <= %f\n", (float)(M_PI/3), angle, (float)(5*M_PI/6));
		   
			     plateinfo *plate = pinfo[a->collision_type];
			cpVect ov = a->body->v;
		   a->body->v = cpvadd(
				cpvsub(a->body->v, cpvproject(a->body->v, a->body->rot)),
				cpvmult( cpvrotate(plate->constv, a->body->rot), (angle<0?1:-1)));
		   bounced = true;

/*		   level[num].trails.push_front(
						trail_info( contacts[c].p, contacts[c].n, 1.0, 1.0, 1.0 ) );
		   level[num].trails.push_front(
						trail_info( a->body->p, cpvforangle(cpvtoangle(a->body->v)), 1.0, 0.0, 1.0 ) );
			level[num].trails.push_front(
					trail_info( a->body->p, cpvforangle(cpvtoangle(ov)), 0.0, 1.0, 1.0 ) );
*/
//		   sball.reset();
//		   ERR("%x %x (%lf) BOUNCE\n", a, b, angle);

			PUSHBACK;
		} else if ( !zoomed && angle < M_PI/3 && angle > -M_PI/3 ) {
		     plateinfo *plate = pinfo[a->collision_type];
			cpVect ov = a->body->v;
			cpVect componentDirection = cpvperp(cpvmult(contacts[c].n, normal_coef));
			cpVect oldComponent = cpvproject(a->body->v, componentDirection);
			
//			ERR("rota %f va %f ol %f\n", cpvtoangle(a->body->rot), cpvtoangle(ov), cpvdot(a->body->rot, ov));
			
			if (cpvlength(oldComponent) < cpvlength(plate->constv)) {
				a->body->v = cpvadd(
					cpvsub(a->body->v, oldComponent),
					cpvmult( cpvrotate(plate->constv, componentDirection), (cpvdot(a->body->rot, ov)>0?1:-1)));
			
/*				level[num].trails.push_front(
					trail_info( contacts[c].p, contacts[c].n, 1.0, 0.0, 0.0 ) );			 
				level[num].trails.push_front(
					trail_info( a->body->p, cpvforangle(cpvtoangle(a->body->v)), 1.0, 0.0, 1.0 ) );			 */
			}
			zoomed = true;
		}
	}
	
	return !C_INVISTYPE(b->collision_type);
}

int sticky_hbounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	bool bounced = false;
	int num = (int)data;
	
	for(int c = 0; !bounced && c < numContacts; c++) {
		cpFloat angle = 
			cpvtoangle(cpvmult(contacts[c].n, normal_coef))
			- a->body->a
			+ M_PI/2; // TODO: World can rotate
		while (angle < -M_PI) angle += 2*M_PI;
		while (angle > M_PI) angle -= 2*M_PI;
		
		if ((angle >= M_PI/6 && angle <= 5*M_PI/6)
			 || (angle <= -M_PI/6 && angle >= -5*M_PI/6)) {
//		ERR("%f <= %f <= %f\n", (float)(M_PI/6), angle, (float)(5*M_PI/6));
		   
			     plateinfo *plate = pinfo[a->collision_type];
			cpVect ov = a->body->v;
		   a->body->v = cpvadd(
				cpvsub(a->body->v, cpvproject(a->body->v, a->body->rot)),
				cpvmult( cpvrotate(plate->constv, a->body->rot), (angle<0?1:-1)));
		   bounced = true;
/*
		   level[num].trails.push_front(
						trail_info( contacts[c].p, contacts[c].n, 1.0, 1.0, 1.0 ) );
		   level[num].trails.push_front(
						trail_info( a->body->p, cpvforangle(cpvtoangle(a->body->v)), 1.0, 0.0, 1.0 ) );
			level[num].trails.push_front(
					trail_info( a->body->p, cpvforangle(cpvtoangle(ov)), 0.0, 1.0, 1.0 ) );
*/
//		   sball.reset();
//		   ERR("%x %x (%lf) BOUNCE\n", a, b, angle);

			PUSHBACK;
		}
	}
	
	return !C_INVISTYPE(b->collision_type);
}

int swoop_bounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	bool hbounced = false;
	bool vbounced = false;
	int num = (int)data; // arrrrgh
     plateinfo *plate = pinfo[a->collision_type];
	
	for(int c = 0; !(hbounced && vbounced) && c < numContacts; c++) {
		cpFloat angle = 
			cpvtoangle(cpvmult(contacts[c].n, normal_coef))
			- a->body->a
			+ M_PI/2; // TODO: World can rotate
		while (angle < -M_PI) angle += 2*M_PI;
		while (angle > M_PI) angle -= 2*M_PI;
		
		if (!hbounced && ((angle >= M_PI/6 && angle <= 5*M_PI/6)
			 || (angle <= -M_PI/6 && angle >= -5*M_PI/6))) {
//		ERR("x %f <= %f <= %f\n", (float)(M_PI/6), angle, (float)(5*M_PI/6));
		   
			cpVect ov = a->body->v;
		   a->body->v = cpvadd(
				cpvsub(a->body->v, cpvproject(a->body->v, a->body->rot)),
				cpvmult( cpvrotate(cpvproject(plate->constv, cpv(1,0)), a->body->rot), (angle<0?1:-1)));
		   hbounced = true;
		   
//		   ERR("conv %f %f -> %f %f\n", ov.x, ov.y, a->body->v.x, a->body->v.y);

//		   level[num].trails.push_front(
//						trail_info( contacts[c].p, contacts[c].n, 1.0, 1.0, 1.0 ) );
//		   level[num].trails.push_front(
//						trail_info( a->body->p, cpvforangle(cpvtoangle(a->body->v)), 1.0, 0.0, 1.0 ) );
//			level[num].trails.push_front(
//					trail_info( a->body->p, cpvforangle(cpvtoangle(ov)), 0.0, 1.0, 1.0 ) );

//		   sball.reset();
//		   ERR("%x %x (%lf) BOUNCE\n", a, b, angle);
		   //return false;
		   
			PUSHBACK;
		}
		
		if (!vbounced && ((angle <= M_PI/3 && angle >= -M_PI/3)
			 || (angle >= 2*M_PI/3 || angle <= -2*M_PI/3))) {
//		ERR("y %f <= %f <= %f\n", (float)(M_PI/4), angle, (float)(3*M_PI/4));
		   cpVect prot = cpvperp(a->body->rot);	   
		   
			cpVect ov = a->body->v;
		   a->body->v = cpvadd(
				cpvsub(a->body->v, cpvproject(a->body->v, prot)),
				cpvmult( cpvrotate(cpvproject(plate->constv, cpv(0,1)), a->body->rot), (fabs(angle)>M_PI/2?-1:1)));
		   vbounced = true;

//		   ERR("conv %f %f -> %f %f\n", ov.x, ov.y, a->body->v.x, a->body->v.y);	

//		   level[num].trails.push_front(
//						trail_info( contacts[c].p, contacts[c].n, 1.0, 1.0, 0.5 ) );
//		   level[num].trails.push_front(
//						trail_info( a->body->p, cpvforangle(cpvtoangle(a->body->v)), 1.0, 0.5, 0.5 ) );
//			level[num].trails.push_front(
//					trail_info( a->body->p, cpvforangle(cpvtoangle(ov)), 0.0, 1.0, 1.0 ) );

//		   sball.reset();
//		   ERR("%x %x (%lf) BOUNCE\n", a, b, angle);
		   //return false;
		   
			PUSHBACK;
		}
	}
	
	return !C_INVISTYPE(b->collision_type);
}

int ing_fallthrough(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	for(int c = 0; c < numContacts; c++) {
		cpFloat angle = 
			cpvtoangle(cpvmult(contacts[c].n, normal_coef))
			- a->body->a
			+ M_PI/2; // TODO: World can rotate
		while (angle < -M_PI) angle += 2*M_PI;
		while (angle > M_PI) angle -= 2*M_PI;
		
		if ((angle >= M_PI/3 && angle <= 5*M_PI/6)
			 || (angle <= -M_PI/3 && angle >= -5*M_PI/6))
			 return true;
	}
	
	return false;
}

int ball_bounce(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	for(int c = 0; c < numContacts; c++) {
		bounces.push_back(&contacts[c]);
	}
	return true;
}

int everybody_dies(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data) {
	deaths.push_back(a);	
	return false;
}

void setupCollisions(spaceinfo &level) {
	  cpSpace *space = level.space;
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_FLOOR, jumpman_lands, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_LOOSE, jumpman_lands, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_LOOSE2, jumpman_lands, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_INVIS, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_LOOSE3, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_LAVA, jumpman_dies, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_MARCH, jumpman_dies, NULL); 
  	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_ING, jumpman_dies, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_STICKY, jumpman_dies, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_SWOOP, jumpman_dies, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_BALL, jumpman_dies, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_ANGRY, jumpman_dies, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_HAPPY, jumpman_lands, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_EXIT , jumpman_exit, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_BACK , jumpman_unexit, NULL); 
  	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_REENTRY, jumpman_checkpoint, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_PAINT , jumpman_paint, (void*)level.num); 
	  
	  unsigned int BALLS[3] = {C_BALL, C_ANGRY, C_HAPPY};
	  for(int c = 0; c < 3; c++) {
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_FLOOR, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_LOOSE, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_LOOSE2, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_LAVA, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_BALL, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_ANGRY, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_HAPPY, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_BOMB, 
			BALLS[c]==C_HAPPY ? jumpman_bombs : ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_BOMB2, 
			BALLS[c]==C_BALL ? jumpman_bombs : ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_EXIT, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_BACK, ball_bounce, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_INVIS, NULL, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_LOOSE3, NULL, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_PAINT, NULL, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_ENTRY, NULL, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_REENTRY, NULL, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_ARROW, NULL, NULL);
		  cpSpaceAddCollisionPairFunc(space, BALLS[c], C_NOROT, NULL, NULL);
		}
	  
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_BALL, everybody_dies, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_BALL, everybody_dies, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_BALL, everybody_dies, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_BALL, everybody_dies, NULL);

	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_HAPPY, everybody_dies, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_HAPPY, everybody_dies, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_HAPPY, everybody_dies, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_HAPPY, everybody_dies, NULL);

	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_PAINT, NULL, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_PAINT, NULL, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_PAINT, NULL, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_PAINT, NULL, NULL);
	  
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_FLOOR, march_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_ANGRY, march_hbounce, (void*)level.num); 
  	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_LOOSE, march_hbounce, (void*)level.num); 
  	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_LOOSE2, march_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_EXIT, march_hbounce, (void*)level.num);  
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_BACK, march_hbounce, (void*)level.num);  
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_BOMB, march_hbounce, (void*)level.num);  
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_BOMB2, march_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_LAVA, march_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_INVIS, march_hbounce, (void*)level.num);
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_LOOSE3, march_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_MARCH, march_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_STICKY, march_hbounce, (void*)level.num); 

	  cpSpaceAddCollisionPairFunc(space, C_ING, C_INVIS, ing_fallthrough, (void*)level.num); 

      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_FLOOR, sticky_hbounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_ANGRY, sticky_hbounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_LOOSE, sticky_hbounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_LOOSE2, sticky_hbounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_EXIT, sticky_hbounce, (void*)level.num);  
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_BACK, sticky_hbounce, (void*)level.num);  
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_BOMB, sticky_hbounce, (void*)level.num);  
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_BOMB2, sticky_hbounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_STICKY, C_LAVA, sticky_hbounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_INVIS, sticky_hbounce, (void*)level.num);
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_LOOSE3, sticky_hbounce, (void*)level.num);
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_STICKY, sticky_hbounce, (void*)level.num);
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_MARCH, sticky_hbounce, (void*)level.num); 

      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_SWOOP, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_ANGRY, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_FLOOR, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_LOOSE, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_LOOSE2, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_EXIT, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_BACK, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_BOMB, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_BOMB2, swoop_bounce, (void*)level.num); 
      cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_LAVA, swoop_bounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_INVIS, swoop_bounce, (void*)level.num); 
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_LOOSE3, swoop_bounce, (void*)level.num); 

	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_BOMB, jumpman_bombs, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_BOMB2, jumpman_lands, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_FLOOR, shrapnel_bounce, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_LAVA,  shrapnel_bounce, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_EXIT,  shrapnel_bounce, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_BACK,  shrapnel_bounce, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_BOMB,  shrapnel_bounce, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_BOMB2,  shrapnel_bounce, NULL);
	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_INVIS, NULL, NULL);

	  // Is there a better way to do this...?
  	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_ENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_ENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_ENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_ENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_ENTRY, NULL, NULL); 
  	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_ENTRY, NULL, NULL); 
	  
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_REENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_REENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_REENTRY, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_REENTRY, NULL, NULL); 
  	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_REENTRY, NULL, NULL); 
	  
  	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_ARROW, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_ARROW, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_ARROW, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_ARROW, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_ARROW, NULL, NULL); 
  	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_ARROW, NULL, NULL); 

  	  cpSpaceAddCollisionPairFunc(space, C_JUMPMAN, C_NOROT, jumpman_fog, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_MARCH, C_NOROT, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_ING, C_NOROT, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_SWOOP, C_NOROT, NULL, NULL); 
	  cpSpaceAddCollisionPairFunc(space, C_STICKY, C_NOROT, NULL, NULL); 
  	  cpSpaceAddCollisionPairFunc(space, C_LOOSE, C_NOROT, NULL, NULL); 
}

void jumpman_reset (bool needsadd)
{
	enemy_info *info = (enemy_info *)chassis->data;

	jumpmanstate = jumpman_normal;
	jumpman_unpinned = false;
	
	  int tilt;
	  if (!level[jumpman_s].repeat || !level[jumpman_s].reprot)
	  	  tilt = 0;
	  else
		tilt = tiltrightnow();
		
		bool reflect = tilt&REPROTREF;
		tilt &= REPROTROT;
	bool needtilt = info->tiltid || tilt;
	
//ERR("Start [%f,%f]x%fx%sx%fx%s ", chassis->p.x, chassis->p.y, chassis->a/M_PI*180, ((enemy_info *)chassis->data)->tiltref?"R":"_", input_power_modifier, surplusSpinning?"Y":"N");
		cpFloat previous_input_power_modifier = input_power_modifier;	
		input_power_modifier = reflect ? -1 : 1;
		if ((previous_input_power_modifier > 0) ^ (input_power_modifier > 0)) {
//			ERR("FROM %lf TO %lf flipping.\n", (double)previous_input_power_modifier, (double)input_power_modifier);
			input_power *= -1;
			input_power_last_facing *= -1;
			input_power_unpinned *= -1;
			input_power_last_facing_unpinned *= -1;
		}
		
		((enemy_info *)chassis->data)->tiltref = reflect;
	chassis->p = cpv(jumpman_x, jumpman_y); //cpvrotate(cpv(jumpman_x, jumpman_y), cpvforangle(tilt*M_PI/2)); // You come in the normal place...
	chassis->v = cpvzero;
	if (cam_fixed == level[jumpman_s].camera)
		cpBodySetAngle(chassis, 0);
	else
		cpBodySetAngle(chassis, (roten-tilt*M_PI/2.0+surplusSpin)*(reflect?-1:1)); // (surplusSpinning?0:surplusSpin)
	info->tiltg = chassis->rot;
//	if (surplusSpinning)
//		((enemy_info *)chassis->data)->tiltg = cpvforangle(roten-tilt*M_PI/2.0); // is this different from cpvneg?

//	ERR("(rot %f tilt %d master %f surplus %lf) into [%f,%f]x%fx%s\n", roten/M_PI*180, tilt, cpvtoangle(level[jumpman_s].master_tiltg)/M_PI*180, surplusSpin/M_PI*180, chassis->p.x, chassis->p.y, chassis->a/M_PI*180, ((enemy_info *)chassis->data)->tiltref?"R":"_");
	
	if (needsadd) {
	  // FIXME: WHAT IF NO SUCH THING AS level[jumpman_s] ?? 
	    SpaceRemoveBody(level[jumpman_s], chassisShape); // Seems like a terrible way to do this, but it's apparently safe?
	  cpSpaceRemoveShape(level[jumpman_s].space, chassisShape);
	} 
	  		
		if (level[jumpman_s].layers > 0)
			chassisShape->layers = (1 << (edit_mode == EWalls ? editLayer : jumpman_l));
		else 
			chassisShape->layers = 0xFFFFFFFF;
	
	if (needsadd) {		
	   retilt(&level[jumpman_s], chassisShape, needtilt?1:0);
	  cpSpaceAddShape(level[jumpman_s].space, chassisShape);
	}
}

void justLoadedCleanup()
{
	wantrot = 0; rotstep = 0; // If we're rotating, stop it.
	surplusSpinning = false; surplusSpin = 0; // If we've been rotating, undo it.
	((enemy_info *)chassis->data)->tiltid = 0;
	if (level[0].haveEntry) {
		jumpman_x = level[0].entry_x;
		jumpman_y = level[0].entry_y;
		jumpman_l = level[0].entry_l;
	}
}

void addSpaceTo(spaceinfo &level) {
	level.space = cpSpaceNew();
  
  /* Next, you'll want to set the properties of the space such as the
     number of iterations to use in the constraint solver, the amount
     of gravity, or the amount of damping. In this case, we'll just
     set the gravity. */
  level.space->gravity = cpvrotate(GRAVITY, cpvforangle(roten));

  /* This step is optional. While you don't have to resize the spatial
  hashes, doing so can greatly increase the speed of the collision
  detection. The first number should be the expected average size of
  the objects you are going to have, the second number is related to
  the number of objects you are putting. In general, if you have more
  objects, you want the number to be bigger, but only to a
  point. Finding good numbers to use here is largely going to be guess
  and check. */
  cpSpaceResizeStaticHash(level.space, 50.0f, 2000);
  cpSpaceResizeActiveHash(level.space, 50.0f, 100);
}

void appendLevel() {
	int s = level.size();
	
	int deep = -1;
	if (s>0)
		deep = level[s-1].deep-level[s-1].after;
	
  /* We first create a new space */
  level.push_back(spaceinfo());
  level[s].num = s;
  level[s].deep = deep;
  
  
  addSpaceTo(level[s]);
	
  /* This is the rigid body that we will be attaching our ground line
     segments to. We don't want it to move, so we give it an infinite
     mass and moment of inertia. We also aren't going to add it to our
     space. If we did, it would fall under the influence of gravity,
     and the ground would fall with it. */
  level[s].staticBody = cpBodyNew(INFINITY, INFINITY);
}

void loadLevel(spaceinfo *s, slice &levelpng, int r, unsigned int l) {
	cpVect off = cpvforangle(r*M_PI/4);
	
	if (!s->repeat_every) s->repeat_every = 36*(levelpng.width-2); // FIXME: Notice the gigantic assumptions I'm making here
	if (s->repeat_every < 0) s->repeat_every = 0;
	
		for(vector<block>::iterator b = levelpng.blocks.begin(); b != levelpng.blocks.end(); b++) {
			  cpVect seg_verts[] = {
				cpv(-18,-18 - 36*(b->height-1)),
				cpv(-18, 18),
				cpv( 18 + 36*(b->width-1), 18),
				cpv( 18 + 36*(b->width-1),-18 - 36*(b->height-1)),
			  };
			  
		for(int c = 0; c < 4; c++)
			seg_verts[c] = cpvrotate(seg_verts[c], off);
		/* Collision shapes are attached to rigid bodies. When the rigid
		   body moves, the collision shapes attached to it move as
		   well. For the ground, we want to attach its collision shapes
		   (the line segments) to our static, non-moving body that we've
		   created. */
		float roundOffset = 0.5; //levelpng.width%2?0.5:0; // Why did I ever do this?
		cpShape *seg;
		cpVect internalCenter; // Normal blocks put their centers in the left corner, shrapnel needs it dead center
		cpVect centerAt = cpv((b->x - levelpng.width/2.0 + roundOffset)*36, -(b->y - levelpng.height/2.0 + roundOffset)*36);
		centerAt = cpvrotate( centerAt, off);
		if (b->color == C_LOOSE) {
			internalCenter = cpvrotate(cpv(-36*(b->width/2.0-0.5), 36*(b->height/2.0-0.5)), off);
			centerAt = cpvsub(centerAt, internalCenter);
		}		
		
		if (b->color == C_LOOSE) {
			cpBody *body = cpBodyNew(INFINITY, INFINITY);
			body->p = centerAt;
			cpFloat m = b->width * b->height * 5;
			cpFloat i = cpMomentForPoly(m, 4, seg_verts, internalCenter);
			body->data = new loose_info(m, i);
			seg = cpPolyShapeNew(body, 4, seg_verts, internalCenter);
		} else { 
			seg = cpPolyShapeNew(s->staticBody, 4, seg_verts, centerAt);
		}

		if (b->color == C_NOROT)
			s->has_norots = true;

		/* After you create a shape, you'll probably want to set some of
		   it's properties. Possibilities include elasticity (e), surface
		   velocity (surface_v), and friction (u). We'll just set the
		   friction. */
		//seg->u = 1.0;
		seg->u = 0.1;
		seg->e = 1.0;
		seg->collision_type = b->color;
		if (s->layers > 0)
			seg->layers = 1 << l;

		/* Lastly, we need to add it to a space for it to do any
		   good. Because the ground never moves, we want to add it to the
		   static shapes to allow Chipmunk to cache the collision
		   information. Adding the line segments to the active shapes
		   would work, but would be slower.  */
		cpSpaceAddStaticShape(s->space, seg);
	}
	
// ---------------   ITERATE STAGES-- LOAD SLICES   ---------------

	plateinfo defaultplate;

	for(int x = 0; x < levelpng.width; x++) {
		for(int y = 0; y < levelpng.height; y++) {
			if (levelpng.pixel[x][y]) {
				unsigned long pixel = levelpng.pixel[x][y];
				unsigned long lrot = pixel & C_META; lrot ^= C_META;
				lrot *= 2; lrot += r;
				cpVect loff = cpvforangle(lrot*M_PI/4);
				pixel |= C_META;
				
				if (C_ENTRY == pixel) {
					s->haveEntry = true;
					s->entry_x = 36*(x - levelpng.width/2.0 + 0.5);
					s->entry_y = -36*(y - levelpng.height/2.0 + 0.5);
					s->entry_l = l;
					// continue; // This breaks edit mode. TODO: Reinstate?
				}
			
				cpVect slice_verts[] = {
					cpv(-18,-18),
					cpv(-18, 18),
					cpv( 18, 18),
					cpv( 18,-18),
				  };

//				cpFloat slice_mass = 5.0f;
//				cpFloat slice_moment = INFINITY;
				plateinfo *plate = pinfo[pixel];
				
				if (!plate) {
//					ERR("Not sure I recognize: %x\n", pixel);
					plate = &defaultplate;
				}

				cpBody *body = cpBodyNew(INFINITY, INFINITY);
				body->p = cpvrotate( cpv(36*(x - levelpng.width/2.0 + 0.5), -36*(y - levelpng.height/2.0 + 0.5)), off);
				body->v = cpvrotate( plate->constv, loff);
				if (plate->defaultdata)
					body->data = plate->defaultdata->clone(); 
				cpBodySetAngle(body, plate->behave==addbehave||(plate->behave==manbehave && okRot(plate->defaultdata))?roten:lrot*M_PI/4);
				
				cpShape *shape;
				
				if (C_BALLTYPE(pixel)) { // Awkward!
					shape = cpCircleShapeNew(body, 36.0, cpvzero);
					shape->e = 0.80;
				} else {
					shape = cpPolyShapeNew(body, 4, slice_verts, cpvzero);
				}
				if (C_SWOOP == pixel || plate->behave == nobehave) { // FIXME: Do not do this
					shape->e = 1.0;
				}
								
				  shape->collision_type = pixel;
				
				  shape->u = plate->u;
					if (s->layers > 0) {
						shape->layers = 1 << l;
					}
					if (shape->collision_type == C_PAINT)
						shape->layers = ~shape->layers;
								
				  switch (plate->behave) {
					default: case nobehave: {
						cpSpaceAddStaticShape(s->space, shape);
					break;} 
					case manbehave: case addbehave: {
						cpBodySetMass(body, 5);
						if (plate->behave == addbehave)
							cpSpaceAddBody(s->space, body);
						if (plate->behave == manbehave) {
							s->mans[(unsigned int)shape] = shape;
							if (body->data)
								((enemy_info *)body->data)->tiltid = -1;
						}
						cpSpaceAddShape(s->space, shape);
					break;}
				  }
			}
		}
	}
// ---------------   DON'T SET UP COLLISIONS.   ---------------  
	
}

void loadLevel(spaceinfo *s, char *filename, int r, unsigned int l) {
		
		slice levelpng;
		
		levelpng.construct(filename, true, r?NULL:s);
		
		loadLevel(s, levelpng, r, l);
  }

void loadLevel(TiXmlNode *levelxml, const char *pathname, bool noFiles) { // Assumes 
	ERR("Descending on %s\n", levelxml->Value());
	
	int s = level.size();			
	
	appendLevel();
	setupCollisions(level[s]);
	
	{ int temp; 
		if (s == 0 || TIXML_SUCCESS == ((TiXmlElement *)levelxml)->QueryIntAttribute("flag", &temp)) {
			flags.push_back(s);
			level[s].flag = flags.size();
		}
	}
	
	((TiXmlElement *)levelxml)->QueryIntAttribute ("layers", &level[s].layers);
	
	for( TiXmlNode *_element = levelxml->FirstChild(); _element; _element = _element->NextSibling() ) {
		if (_element->Type() != TiXmlNode::ELEMENT) continue;
		TiXmlElement *element = (TiXmlElement *)_element;
		string value = element->ValueStr();
		ERR("Processing on %s\n", value.c_str());
		
		if (value == "Type") {
			int camera, after, rots, dontrot, rspeed, repeat, reprot; double zoom;
			if (TIXML_SUCCESS != element->QueryIntAttribute ("camera", &camera)) {
				camera = 0;
			}
			if (TIXML_SUCCESS != element->QueryIntAttribute ("after", &after)) {
				after = 1;
			}
			if (TIXML_SUCCESS != element->QueryDoubleAttribute ("zoom", &zoom)) {
				zoom = 1;
			}
			if (TIXML_SUCCESS != element->QueryIntAttribute ("repeat", &repeat)) {
				repeat = 0;
			}
			if (TIXML_SUCCESS != element->QueryIntAttribute ("rots", &rots)) {
				rots = 0;
			}
			if (TIXML_SUCCESS != element->QueryIntAttribute ("dontrot", &dontrot)) {
				dontrot = 0;
			}
			if (TIXML_SUCCESS != element->QueryIntAttribute ("rspeed", &rspeed)) {
				rspeed = 1;
			}
			if (TIXML_SUCCESS == element->QueryIntAttribute ("reprot", &reprot)) {
				level[s].reprot = true;
				level[s].reprots = reprot;
				
				// This can't possibly be the smartest way to do this, but I can't think of anything else.
				// "Reprots" describes the side identification & twistedness characteristics of each side of the square.
				// But generally we're more interested in the mapping (x,y)->[reflect, rotate 0-3].
				// So we build the latter out of the former by just making a cache of (0,0) to (4,4), which we assume repeats endlessly:
				level[s].disprot[0][0] = 0; // Tragically, we now have three different algorithms for doing this same thing, in different places, all of which were experimentally derived.
				for(int x = 1; x < 4; x++) {
					unsigned int tilt = level[s].disprot[x-1][0];
					int rot = (tilt&REPROTROT);
					int ref = tilt&REPROTREF;
					rot = rot + 1;
					unsigned int tilt2 = level[s].getReprot(rot%4);
					rot = (tilt&REPROTROT);
					if ((tilt2 == 5 || tilt2 == 7) && rot < 2) rot += 2; // I have no idea why this is even needed, but something similar is in wrapObject
					int rot2 = (tilt2&REPROTROT);
					int ref2 = tilt2&REPROTREF;
					level[s].disprot[x][0] = ((4+rot-rot2)&REPROTROT) | (ref^ref2);
				} 
				for(int y = 1; y < 4; y++) {
					for(int x = 0; x < 4; x++) {
						unsigned int tilt = level[s].disprot[x][y-1];
						int rot = (tilt&REPROTROT);
						int ref = tilt&REPROTREF;
						unsigned int tilt2 = level[s].getReprot(rot%4);
						rot = (tilt&REPROTROT);
						if ((tilt2 == 5 || tilt2 == 7) && (1!=rot && 2!=rot)) rot += 2; // I have no idea why this is even needed
						int rot2 = (tilt2&REPROTROT);
						int ref2 = tilt2&REPROTREF;
						level[s].disprot[x][y] = ((4+rot-rot2)&REPROTROT) | (ref^ref2);
					}
				}
			} else { // This last bit's probably redundant, the constructor should fill these..
				level[s].reprot = false;
				level[s].reprots = 0;
			}
			{
				const char *fallout = element->Attribute("fallout");
				if (NULL != fallout) {
					level[s].fallout = true;
					level[s].falloutMessage = fallout;
				}
			}
			{
				int temp;
				level[s].panProblem = TIXML_SUCCESS == element->QueryIntAttribute ("panproblem", &temp);
			}
			level[s].camera = (camera_type)camera;
			level[s].after = after;
			level[s].zoom = zoom;
			level[s].repeat = (repeat_type)repeat;
			level[s].rots = rots;
			level[s].orots = rots;
			level[s].dontrot = dontrot;
			level[s].odontrot = dontrot;
			level[s].rspeed = rspeed;
			
			// Arrgh I'm not really sure this should go here.
			if (optSlow && level[s].after > 2)
				level[s].after = 2; // Or maybe I should just divide by two when >= 2? I'm just not sure.
		} else if (value == "Color") { // This probably is the kind of thing that shouldn't be happening out side of Main.cpp
			int l = 0;
			element->QueryIntAttribute ("layer", &l);
			if (l>MAXLAYERS) l=0;
			
			if (TIXML_SUCCESS == element->QueryDoubleAttribute ("r", &level[s].r[l])) {
				element->QueryDoubleAttribute ("r", &level[s].r[l]);
				element->QueryDoubleAttribute ("g", &level[s].g[l]);
				element->QueryDoubleAttribute ("b", &level[s].b[l]);
			} else {
				double h, sat, v; // s vs sat were aliasing each other
				
				element->QueryDoubleAttribute ("h", &h);
				element->QueryDoubleAttribute ("s", &sat);
				element->QueryDoubleAttribute ("v", &v);
				HSVtoRGB(&level[s].r[l], &level[s].g[l], &level[s].b[l], h, sat, v);
			}

		} else if (value == "File") {
			if (!noFiles) {
				int l = 0;
				element->QueryIntAttribute ("layer", &l);
			
				char filename2[FILENAMESIZE];
				int rot;
				snprintf(filename2, FILENAMESIZE, "%s/%s", pathname, element->Attribute("src"));
				if (TIXML_SUCCESS != element->QueryIntAttribute ("rot", &rot)) {
					rot = 0;
				}
				
				loadLevel(&level[s], filename2, rot, l);
			}
		} else if (value == "Message") {
			string text = element->Attribute("text");
			level[s].messages.push( text.c_str() );
		} else if (value == "Flag") { // Uh hell I dunno.
		}
	}
}

void eraseTrailingZeroes(char *str) {
	int len = strlen(str);
	for(int c = len-1; c >= 0 && str[c] == '0'; c--) {
		if (!(c-1 > 0 && str[c-1] == '.'))
			str[c] = '\0';
	}
}

// Assumes RootElement already checked
void loadGame(TiXmlDocument &xml, const char *pathname) {
	{	// First read the attributes of the root "Jumpman" element.
		double version; int easyMode;
		if (TIXML_SUCCESS == xml.RootElement()->QueryDoubleAttribute ("Version", &version)) {
			if (version > COMPILED_VERSION) { // This game demands a newer version of the game than we have installed
				char filename2[FILENAMESIZE];
				string versionMessage; // This is tricky becuase printf number formatting kinda sucks.
				
				versionMessage = "This level pack requires Jumpman version ";
				snprintf(filename2, FILENAMESIZE, "%lf", version);
				eraseTrailingZeroes(filename2);
				versionMessage += filename2;
				versionMessage += ".\nRight now you're using version ";
				snprintf(filename2, FILENAMESIZE, "%lf", (double)COMPILED_VERSION);
				eraseTrailingZeroes(filename2);
				versionMessage += filename2;
				versionMessage += ".\n";

				BombBox(versionMessage);
			}
		}
		if (TIXML_SUCCESS == xml.RootElement()->QueryIntAttribute("easier", &easyMode)) {
			doInvincible = easyMode;
		}
		xml.RootElement()->QueryIntAttribute("ending", &desiredEnding); // If this fails: We don't care
	}

   for( TiXmlNode *levelxml = xml.RootElement()->FirstChild(); levelxml; levelxml = levelxml->NextSibling() ) {
	if (levelxml->Type() != TiXmlNode::ELEMENT || levelxml->ValueStr() != "Level") continue;
	loadLevel(levelxml, pathname);
	}

//	   loadLevel(&level[0], filename, 1);
	if (level.size() > 0) {
		jumpman_s = 0;
		jumpman_d = level[jumpman_s].deep+1;
		rePerspective = true;
		justLoadedCleanup();
	} else {
		BombBox("The level pack appears to be horribly corrupted.");
	}
}

void loadGame(const char *filename) {
	   char filename2[FILENAMESIZE];
	   snprintf(filename2, FILENAMESIZE, "%s/index.xml", filename);

	   TiXmlDocument xml(filename2);
	   xml.LoadFile();
	   
	   if (!xml.RootElement()) {
		REALERR("Couldn't load file %s somehow?\n", filename2);
		FileBombBox(filename2);
	   }
	   loadGame(xml, filename);
}

void loadEditorFile(const char *filename) {
	   char filename2[FILENAMESIZE];
	   snprintf(filename2, FILENAMESIZE, "%s/index.xml", filename);
	   
		editingPath = filename;
		if (editing) delete editing;
		editing = new TiXmlDocument(filename2);
	   editing->LoadFile();
	   
	   if (!editing->RootElement()) {
		REALERR("Couldn't load file %s somehow?\n", filename2);
		FileBombBox(filename2);
	   }
}

void dummyStage() {
  appendLevel();
	level[0].camera = cam_fixed;	 
	level[0].r[0] = 0; level[0].g[0] = 0.5; level[0].b[0] = 1.0;
	level[0].rots = 0xFF; 
	setupCollisions(level[0]);
}

/* The init funtion is doing most of the work in this tutorial. We
   create a space and populate it with a bunch of interesting
   objects. */
void
moonBuggy_init(void)
{

// ---------------   STRUCTURE SETUP ---------------

// All defaultdatas must be enemy_info subclasses

pinfo[C_JUMPMAN] = plate_construct("jumpman1 %d.png", 4, 1.0f,0.5f,0.0f, nobehave);
pinfo[C_EXIT] = plate_construct("exit.png", 1, 0.0f,0.5f,1.0f, nobehave);
	pinfo[C_EXIT]->u = 0.1;
pinfo[C_BACK] = plate_construct("exit2.png", 1, 0.0f,0.5f,1.0f, nobehave);
pinfo[C_ARROW] = plate_construct("arrow.png", 1, 1.0f,1.0f,1.0f, nobehave);
pinfo[C_BOMB] = plate_construct("kyou_bomb 1.png", 1, 1.0f,0.5f,0.0f, nobehave);
	pinfo[C_BOMB]->u = 0.1;
pinfo[C_BOMB2] = plate_construct("kyou_bomb %d.png", 2, 1.0f,1.0f,0.0f, nobehave);
	pinfo[C_BOMB2]->u = 0.1;
pinfo[C_PAINT] = plate_construct("paintbrush.png", 1, 1.0f,1.0f,1.0f, nobehave);
pinfo[C_MARCH] = plate_construct("kyou_spiny %d.png", 2, 0.5f,1.0f,0.0f, addbehave);
	pinfo[C_MARCH]->constv = cpv(100.0,0); pinfo[C_MARCH]->u = 0;
	pinfo[C_MARCH]->defaultdata = new enemy_info();
pinfo[C_OUTLINE] = plate_construct("invisible.png", 1, 0.5f,0.5f,0.5f, nobehave);
pinfo[C_OUTLINE2] = plate_construct("invisible_entry.png", 1, 0.5f,0.5f,0.5f, nobehave);
pinfo[C_UNKNOWN] = plate_construct("invisible_unknown.png", 1, 0.5f,0.5f,0.5f, nobehave);
pinfo[C_ING] = plate_construct("eyes5_hunter %d.png", 2, 0.5f,1.0f,0.0f, addbehave);
	pinfo[C_ING]->defaultdata = new ing_info(0);
pinfo[C_STICKY] = plate_construct("kyou_sticky %d.png", 2, 0.5f,1.0f,0.0f, manbehave);
	pinfo[C_STICKY]->constv = cpv(100.0,0);
	pinfo[C_STICKY]->defaultdata = new enemy_info(true);
pinfo[C_SWOOP] = plate_construct("kyou_swoopy %d.png", 2, 0.5f,1.0f,0.0f, manbehave);
	pinfo[C_SWOOP]->constv = cpv(50*sqrt(2.0),50*sqrt(2.0));
	pinfo[C_SWOOP]->defaultdata = new enemy_info();
pinfo[C_BALL] = plate_construct("ball2.png", 1, 1.0f,1.0f,0.0f, addbehave);
	pinfo[C_BALL]->defaultdata = new enemy_info();
pinfo[C_ANGRY] = plate_construct("ball_sad2 %d.png", 2, 0.5f,1.0f,0.0f, addbehave);
	pinfo[C_ANGRY]->defaultdata = new enemy_info();
pinfo[C_HAPPY] = plate_construct("ball_happy2 %d.png", 2, 1.0f,0.5f,0.0f, addbehave);
	pinfo[C_HAPPY]->defaultdata = new enemy_info();
pinfo[C_BIRD] = plate_construct("m %d.png", 3, 0.0f,0.0f,0.0f, addbehave);

construct_mountain();

// ---------------   DUMMY STAGE   ---------------
  
  dummyStage();
  
// ---------------   CREATE JUMPMAN   ---------------

  /* These are the vertexes that will be used to create the buggy's
     chassis shape. You *MUST* specify them in a conterclockwise
     order, and they *MUST* form a convex polygon (no dents). If you
     need a non-convex polygon, simply attach more than one shape to
     the body. */
  cpVect chassis_verts[] = {
    cpv(-15.75,-18),
    cpv(-15.75, 18),
    cpv( 15.75, 18),
    cpv( 15.75,-18),
  };
	
  cpFloat chassis_mass = 5.0f;

  /* The moment of inertia (usually written simply as 'i') is like the
     mass of an object, but applied to its rotation. An object with a
     higher moment of inertia is harder to spin. Chipmunk has a couple
     of helper functions to help you calculate these. */
#if ALLOW_SPINNING
  cpFloat chassis_moment = cpMomentForPoly(chassis_mass, 4, chassis_verts, cpvzero);
#else
  cpFloat chassis_moment = INFINITY;
#endif

	jumpman_s = 0;
	jumpman_d = level[jumpman_s].deep+1;
	jumpman_x = -36* 5;
	jumpman_y = -36 * 5;
	
  /* Create the rigid body for our buggy with the mass and moment of
     inertia we calculated. */
  chassis = cpBodyNew(chassis_mass, chassis_moment);

	chassis->data = new enemy_info();

  /* Like usual, after something, you'll want to set it
     properties. Let's set the buggy's location to be just above the
     start of the terrain. */
//  chassis->p = cpv(1000.0, 1000.0);
//  chassis->p = cpv(100.0f, 1000.0f);
	
  /* Now we need to attach collision shapes to the chassis and
     wheels. The shapes themselves contain no useful information to
     you. Normally you'd keep them around simply so that you can
     remomve them from the space, or free them later. In this tutorial
     we won't be removing anything, and we're being lax about memory
     management. So we'll just recycle the same variable. */
  cpShape *shape;
	
  /* We create a polygon shape for the chassis. */
  shape = cpPolyShapeNew(chassis, 4, chassis_verts, cpvzero);
  shape->u = 0.5;
  shape->collision_type = C_JUMPMAN;
  chassisShape = shape; 
  jumpman_reset();
  
// ---------------   THE END   ---------------

}

void
moonBuggy_input(int button, int state, int x, int y)
{
  input_power = (state ? 0.0 : 1.0);
}

bool arrows[2] = {false, false};

void serviceArrows(char preference = 0) {
	cpFloat old_input_power = input_power;
	if (arrows[0] || (arrows[0] && arrows[1] && LEFT==preference)) {
		input_power = -1.0 * input_power_modifier;
	} else if (arrows[1]) {
		input_power = 1.0 * input_power_modifier;
	} else {
		input_power = 0;
	} 
	if (input_power) {
		input_power_last_facing = input_power; // Preserve direction of movement FOR ALL TIME
		if (!old_input_power) { // If we just started moving
			started_moving_at_ticks = ticks;
		}
	}
}

void moonBuggy_keydown(unsigned char key) {
	bool wasarrow = false;
	if (jumpman_unpinned && (key == JUMP /* || key == ROTL || key == ROTR */)) return;
	cpFloat old_input_power = input_power; // I have written myself into a corner.
	cpFloat old_input_power_last_facing = input_power_last_facing;
	
//	ERR("DOWN %c\n", key);
	switch(key) {
		case JUMP: 
			if (on_land && !paused) {
				cpVect blah = {0.0, 500};
				blah = cpvrotate(blah, chassis->rot);
				chassis->v = cpvadd(chassis->v, blah);
				sjump.reset();
				jumping = true;
			}
			break;
		case LEFT:
			arrows[0] = true;
			wasarrow = true;
			break;
		case RIGHT:
			arrows[1] = true;
			wasarrow = true;
			break;
		case ROTL: case ROTR:
//			if (input_power_modifier < 0) {
//				key = (ROTL == key ? ROTR: ROTL);
//			}
			if (pantype != pan_deep && !doingEnding) {
				bool rotDisabled = level[jumpman_s].rots == 0 || level[jumpman_s].rots == 1;
				if (edit_mode != EWalls &&
					(fogged
					|| rotDisabled
					|| (key == ROTL && (level[jumpman_s].dontrot & 0x80))
					|| (key == ROTR && (level[jumpman_s].dontrot & 0x02)))) {
					sland.reset();
					rotYell(fogged || rotDisabled, fogged);
				} else {
					do {
						if (key == ROTL) { // Maybe this is backward.
							wantrot++;
							rotl(level[jumpman_s].rots);
							rotl(level[jumpman_s].dontrot);
						} else {
							wantrot--;
							rotr(level[jumpman_s].rots);
							rotr(level[jumpman_s].dontrot);
						}
					} while ( edit_mode != EWalls && !(level[jumpman_s].rots & 1) ); // No rotation limits in the editor
				}
				if (!fogged)
					anglingSince = ticks;
			}
			break;
		case QUIT:
			if (BackOut() && !doingEnding) {
				slick.w = 1;
				slick.reset();
			}
			break; // heh
		default:
			break;
	}
	if (wasarrow) serviceArrows(key);
	
	input_power_unpinned = input_power;
	input_power_last_facing_unpinned = input_power_last_facing;
	if (jumpman_unpinned) { // If jumpman is "unpinned", we want to discard whatever that keystroke was after cacheing it.	
		input_power = old_input_power;
		input_power_last_facing = old_input_power_last_facing;
	}
}

void moonBuggy_keyup(unsigned char key) {
	bool wasarrow = false;
//	ERR("UP   %c\n", key);
	switch(key) {
		case ' ':
			break;
		case LEFT:
			arrows[0] = false;
			wasarrow = true;
			break;
		case RIGHT:
			arrows[1] = false;
			wasarrow = true;
			break;
		case CONTROLS:
			if (!doingEnding) {
				controlsScreen();
				slick.w = 1;
				slick.reset();
			}
			break;
		default:
			break;
	}
	if (wasarrow) serviceArrows();
	
	input_power_unpinned = input_power;
	input_power_last_facing_unpinned = input_power_last_facing;
}

void splode_info::adjust_sound() {
	int desired = 100 + ct;
	if (splodey.w > desired || splodey.ticks <= 0) {
		splodey.w = 100 + ct;
	}
}

splode_info::splode_info(spaceinfo *space, cpShape *shape, int frame, cpVect _p, bool _reflect)
		: p(_p), reflect(_reflect), ct(0), tt(100)
{
	plateinfo *plate = pinfo[shape->collision_type];

	// Code duplication grr
	bool forceL = space->layers > 0;
	if (forceL) {
		unsigned int l = 0;
		unsigned int layers = shape->layers;
		if (shape->collision_type == C_PAINT)
			layers = ~layers;
			
		for(int c = 0; c < MAXLAYERS; c++) {
			if ((1<<c) & layers) {
				l = c;
				break;
			}
		}
		
		r = space->r[l]; g = space->g[l]; b = space->b[l];
	} else {
		r = plate->r; b = plate->b; g = plate->g;
	}
	
	if (forceL && optColorblind)
		BlindColorTransform(r, g, b, shape->layers);
	
	sploding = plate->slices[frame];
	
	adjust_sound();
	splodey.max = 30000;
	splodey.reset();
	
}

trail_info::trail_info(cpVect _at, cpVect _trail, double _r, double _g, double _b)
		: at(_at), trail(_trail), r(_r), g(_g), b(_b), ct(0), tt(100) { }

/* This function is called everytime a frame is drawn. */
void
moonBuggy_update(void)
{
	if (paused && edit_mode == ENothing) return;

	// Before we do ANYTHING, handle rotation
	if (wantrot || rotstep) {
		int dir = rotstep < wantrot*ROTSTEPS ? 1 : -1;
			
		//ERR ("\trs %d wr %d\tss %s _ tr %s im %s\n", rotstep, wantrot, surplusSpinning?"Y":"N", ((enemy_info *)chassis->data)->tiltref?"Y":"N", input_power_modifier<0?"Y":"N");
		
		rotenDelta( ( -M_PI/4 ) * level[jumpman_s].rspeed * (double)dir/ROTSTEPS);
		rotstep += dir*level[jumpman_s].rspeed;
		if (rotstep >= ROTSTEPS) {
			rotstep = 0;
			wantrot--;
		} else if (rotstep <= -ROTSTEPS) {
			rotstep = 0;
			wantrot++;
		}
	}
	
	if (paused) return;

  /* Collision detection isn't amazing in Chipmunk yet. Fast moving
     objects can pass right though eachother if they move to much in a
     single step. To deal with this, you can just make your steps
     smaller and cpSpaceStep() sevral times. */
  int substeps = SLOWMO?1:3;

  /* This is the actual time step that we will use. */
  cpFloat dt = (1.0f/FPS) / (cpFloat)substeps;
	
  int i;
  for(i=0; i<substeps; i++){
    /* In Chipmunk, the forces and torques on a body are not reset
       every step. If you keep accumulating forces on an object, it
       will quickly explode. Comment these lines out to see what I
       mean. This function simply zeros the forces and torques applied
       to a give body. */
    cpBodyResetForces(chassis);
		
    /* We need to calculate how much torque to apply to the wheel. The
       following equation roughly simulates a motor with a top
       speed. */
//    cpFloat max_w = -100.0f;
    //cpFloat torque = 60000.0f*fminf((chassis->w - input_power*max_w)/max_w, 1.0f);
	cpVect force = {input_power*1000*chassis->m,0};
	force = cpvrotate(force, chassis->rot);
	cpVect offset = {0,0};

    //cpvadd(chassis->f, force);
	cpBodyApplyForce(chassis, force, offset);

    /* Finally, we step the space */
	was_on_land = on_land;
	on_land = false;
	fogged = false; // If true will be re-set below
	  for(vector<spaceinfo>::iterator _s = level.begin(); _s != level.end(); _s++) {
		spaceinfo *s = &(*_s);
		if (okayToDraw(s->deep)) {		
			// Iterate space normally
			cpSpaceStep(s->space, dt);
			
			// Iterate zero-g objects manually
			for(hash_map<unsigned int, cpShape *>::iterator b = s->mans.begin(); b != s->mans.end(); b++) {
				cpBodyUpdateVelocity((*b).second->body, cpvzero, 1, dt);
				cpBodyUpdatePosition((*b).second->body, dt);
			}

			if (s->anytilts) { // tilt-g objects too				
				cpFloat g = cpvlength(s->space->gravity);
				for(hash_map<unsigned int, cpShape *>::iterator b = s->tilt.begin(); b != s->tilt.end(); b++) {
					enemy_info *info = (enemy_info *)(*b).second->body->data;
					cpVect g2 = cpvmult(cpvneg(cpvperp(info->tiltg)), g); // Maybe it would be better to do this with cpvrotate.
					cpBodyUpdateVelocity((*b).second->body, g2, 1, dt);
					cpBodyUpdatePosition((*b).second->body, dt);
				}
			}
						
			// Post-collision cleanup
			if (bombplease) {
				shatter.w = 20;
				shatter.reset();
				cpSpaceRemoveStaticShape(s->space, bombplease);
				bombplease = NULL;
				cpSpaceHashEach(s->space->staticShapes, &shakeLoose, NULL);
				for(vector<cpShape *>::iterator b = shaking.begin(); b != shaking.end(); b++) {
					cpSpaceRemoveStaticShape(s->space, (*b));
					loose_info *info = (loose_info *)(*b)->body->data;
					cpBodySetMass((*b)->body, info->m);
					cpBodySetMoment((*b)->body, info->i);
					info->noRot = true;
					(*b)->e = 0.5;
					info->tiltg = s->master_tiltg;
					cpSpaceAddBody(s->space, (*b)->body);
					cpSpaceAddShape(s->space, (*b));
				}
				for(vector<cpShape *>::iterator b = shaking2.begin(); b != shaking2.end(); b++) {
//					cpSpaceRemoveStaticBody(s->space, (*b)->body); // Seems like a terrible way to do this, but it's apparently safe?
					cpSpaceRemoveStaticShape(s->space, *b);
				}

				shaking.clear();
				shaking2.clear();
			}
			if (bounces.size() > 0) {
//				  ERR("BALLS\n");
				  for(vector<cpContact *>::iterator b = bounces.begin(); b != bounces.end(); b++) {
					cpContact *contact = *b;
//					ERR("BALL: d %lf m %lf,%lf b %lf; jt %lf, %lf, %lf\n", (double)contact->dist, (double)contact->nMass, (double)contact->tMass, (double)contact->bounce, (double)contact->jnAcc, (double)contact->jtAcc, (double)contact->bias);
					if (fabs(contact->jnAcc) > 300)
						sball.reset();

				  }			
			
				bounces.clear();
			}
			
			if (shatters.size() > 0) {
			  cpFloat loudest = 0;
			  for(vector<cpContact *>::iterator b = shatters.begin(); b != shatters.end(); b++) {
				cpContact *contact = *b;
#define SHRAPMIN 1000.0
#define SHRAPMAX 25000.0
//					ERR("SHATTER: d %lf m %lf,%lf b %lf; jt %lf, %lf, %lf\n", (double)contact->dist, (double)contact->nMass, (double)contact->tMass, (double)contact->bounce, (double)contact->jnAcc, (double)contact->jtAcc, (double)contact->bias);
				if (fabs(contact->jnAcc) > SHRAPMIN) {
					loudest = fabs(contact->jnAcc);
				}

			  }	
			  if (loudest > 0) {
//  					cpFloat logLoud = log(loudest) - log(SHRAPMIN);
					cpFloat ratio = (loudest-SHRAPMIN) / (SHRAPMAX-SHRAPMIN); if (ratio > 1) ratio = 1;
					shatter.w = 200 - ratio*170; if (shatter.w <= 0) shatter.w = 1;
//					ERR("Shatter Loudest %lf log %lf ratio %lf shatter %d\n", (double)loudest, (double) logLoud, (double)ratio, (int)shatter.w); 
					shatter.reset();
			  }		
			  shatters.clear();
			}
			
			if (deaths.size() > 0) {
				for(vector<cpShape *>::iterator b = deaths.begin(); b != deaths.end(); b++) {
					enemy_info *info = (enemy_info *)(*b)->body->data;
				
					s->splodes.push_front(
						splode_info( s, *b, info->lastWhichFrame, (*b)->body->p, info->lastReflect ) );

					if ((*b)->body->data)
					    SpaceRemoveBody(*s, (*b)); // Seems like a terrible way to do this, but it's apparently safe?
					else
					  cpSpaceRemoveBody(s->space, (*b)->body); // Seems like a terrible way to do this, but it's apparently safe?
					cpSpaceRemoveShape(s->space, *b);
				}
				deaths.clear();
			}
			
			// Correct wraparounds
			if (s->repeat) {
				cpSpaceHashEach(s->space->activeShapes, &wrapObject, s);
			}
		}
	  }
	
		// FIXME: USED TO IT LOOKED LIKE THIS WENT FURTHER THAN IT SHOULD HAVE?!
		// update: ... what does this comment mean?
	  switch (jumpmanstate) { // Cleanup deaths, disconnections, etc.
		case jumpman_wantsplode:
			jumpmanstate = jumpman_splode;
			jumpman_unpinned = true;
			
			if (!haveTimerData) {
				timerLives++;
				subTimerLives++;
			}
			
			level[jumpman_s].splodes.push_front(
				splode_info( &level[jumpman_s], chassisShape, currentJumpmanFrame(), chassis->p, input_power_last_facing > 0 ) );

			  SpaceRemoveBody(level[jumpman_s], chassisShape);
			cpSpaceRemoveShape(level[jumpman_s].space, chassisShape);
			jumpct = 0;
			jumptt = 100;
			
			pantt = 0;
			break;
		case jumpman_wantexit: {
			const int e = exit_direction;
			jumpmanstate = jumpman_pan;
			jumpman_unpinned = true;
			  SpaceRemoveBody(level[jumpman_s], chassisShape);
			pantype = pan_deep;
			
//			ERR("Okay rescan: %lf, %lf\n", (double)rescan.x, (double)rescan.y); // FIXME: NOTILTID
				
			if (level[jumpman_s].camera == cam_fixed) {
				panf = cpvzero;
			} else {
				panf = chassis->p;
				
				int reprot = tiltrightnow(); // notice, we haven't altered rescan yet
				cpVect rotate = cpvforangle((reprot&REPROTROT)*M_PI/2);
				bool reflect = reprot&REPROTREF;
				if (reflect) panf.x = -panf.x; panf = cpvrotate(panf, rotate);
			}
			
			panfz = jumpman_d;
			
			// Adjust rescan to fit
			if (edit_mode == EWalls || !(rescan.x || rescan.y) || (jumpman_s+e < level.size() &&
				(level[jumpman_s+e].camera == cam_fixed || !level[jumpman_s+e].repeat))) {
					panf = cpvsub(panf, rescan);
					rescan = cpvzero;
			} else if (jumpman_s+e < level.size()) {
				cpVect oldrescan = rescan;
				
				rescan.x = round_to(rescan.x, level[jumpman_s+e].repeat_every);
				rescan.y = round_to(rescan.y, level[jumpman_s+e].repeat_every); 

				panf = cpvadd(panf, cpvsub(rescan, oldrescan));
			}
			
			if (edit_mode == EWalls || (jumpman_s+e < level.size() && level[jumpman_s+e].camera == cam_fixed)) {
				pant = cpvzero;
			} else if (jumpman_s+e < level.size() && level[jumpman_s+e].haveEntry) {
				pant = cpv(level[jumpman_s+e].entry_x, level[jumpman_s+e].entry_y);
			} else {
				pant = chassis->p;
			}
			if (pant.x || pant.y) {
				int whichlevelto = jumpman_s+(jumpman_s+e<level.size()?e:0);
				int reprot = level[whichlevelto].tiltfor(floor(rescan.x/level[whichlevelto].repeat_every+0.5), floor(rescan.y/level[whichlevelto].repeat_every+0.5));
				cpVect rotate = cpvforangle((reprot&REPROTROT)*M_PI/2);
				bool reflect = reprot&REPROTREF;
				if (reflect) pant.x = -pant.x; pant = cpvrotate(pant, rotate);
			}
			
			if (e>0)
				pantz = jumpman_d - level[jumpman_s].after;
			else
				pantz = jumpman_d + level[jumpman_s-1].after;
			
			panfr = rFor(level[jumpman_s]);
			pantr = jumpman_s+e < level.size() ? rFor(level[jumpman_s+e]) : panfr;
			pantt = 60*(1 + (level[jumpman_s].after-1)*0.25); panct = 0; // ...zoom slower for distant levels but, not in a 1:1 way
			splodey.max = 60000;
			splodey.reset();
			
			exiting_entire_game = jumpman_s+e >= level.size() && desiredEnding > 0;
			if (exiting_entire_game) { // Trigger ending?
				pantz = 0;
				pant.x = 0;
				pant.y = 0;
				pantt = 60*(1 + level.size() / 25);
				splodey.max = 44100*6;
				splodey.reset();
			}
		} break;
		default: break;
	  }
	
  }
  
  if (jumpmanstate == jumpman_splode) {
		jumpct++;
		
		if (0 == pantt && jumpct >= jumptt/4) {
			pantype = pan_side;
			if (edit_mode == EWalls || level[jumpman_s].camera == cam_fixed) {
				panf = cpvzero; pant = cpvzero;
			} else {
				 int reprot = tiltrightnow();
				 cpVect rotate = cpvforangle((reprot&REPROTROT)*M_PI/2);
				 bool reflect = reprot&REPROTREF;
			
				panf = chassis->p;
				if (reflect) panf.x = -panf.x; panf = cpvrotate(panf, rotate);
				pant = cpv(jumpman_x, jumpman_y); 
				if (reflect) pant.x = -pant.x; pant = cpvrotate(pant, rotate);
			}
			panfz = jumpman_d; pantz = jumpman_d;
			panfr = rFor(level[jumpman_s]); pantr = panfr;
			pantt = 60; panct = 0;
		}
		if (jumpct > jumptt) {
			jumpmanstate = jumpman_pan;
		}
	}
	if (pantype != pan_dont) {
		panct++;
		if (pan_deep == pantype) {
			if (exit_direction > 0)
				splodey.w = 90 - panct;
			else
				splodey.w = 30 + panct;
			if (exiting_entire_game && splodey.w <= 7) splodey.w = 8;
			else if (splodey.w <= 0) splodey.w = 1;
		}
		if (panct > pantt) {
			if (pan_deep == pantype) {
				cpSpaceRemoveShape(level[jumpman_s].space, chassisShape);
				if (cam_fixed != level[jumpman_s].camera) { // Just in case "fall" ever gets used
					rotenDelta(-roten);
					level[jumpman_s].rots = level[jumpman_s].orots; // In case we ever unexit		
					level[jumpman_s].dontrot = level[jumpman_s].odontrot;
				}
				roten = 0; surplusSpinning = false; surplusSpin = 0;
				wantrot = 0; rotstep = 0; // If we're rotating, stop it.
				((enemy_info *)chassis->data)->tiltid = 0;
				if (exit_direction > 0) {
					jumpman_d -= level[jumpman_s].after;
					jumpman_s++;
				} else {
					jumpman_d += level[jumpman_s-1].after;
					jumpman_s--;
				}
				splodey.ticks = 0;
				if (level[jumpman_s].haveEntry) {
					jumpman_x = level[jumpman_s].entry_x;
					jumpman_y = level[jumpman_s].entry_y;
					jumpman_l = level[jumpman_s].entry_l;
				} else {
					cpVect p = chassis->p;
					jumpman_x = p.x;
					jumpman_y = p.y;
					jumpman_l = 0;
				}
				
				if (jumpman_s >= level.size() || level[jumpman_s].flag)
					timer_flag_rollover();
					
				doingInvincible = false; // Because why not.
				
				anglingSince = ticks;
			} else {
				if (doInvincible)
					doingInvincible = true;
				lastRebornAt = ticks;
			}
			input_power = input_power_unpinned;
			input_power_last_facing = input_power_last_facing_unpinned;
			pantype = pan_dont;
			if (jumpman_s < level.size())
				jumpman_reset();
			else {
				completeTimer();
				videoEnd();
				if (desiredEnding >= 2) {
					startEnding();
				} else {
					want = onWin;
					wantClearUi = true;
					serviceInterface();
				}
			}
		}
	}
	
	if (edit_mode == EPlayground) {
		if ( chassis->p.x*chassis->p.x + chassis->p.y*chassis->p.y > 1000*1000*aspect*aspect*1.5*1.5 ) { // FIXME: BETTER NUMBER
			jumpman_reset(false);
		}
	}
	
	// On levels with "fallout" set, die when jumpman... falls out
	if (level.size() && level[jumpman_s].fallout && !level[jumpman_s].repeat && !jumpman_unpinned &&
		chassis->p.x*chassis->p.x + chassis->p.y*chassis->p.y > level[jumpman_s].base_width*level[jumpman_s].base_width*2) {
		jumpmanstate = jumpman_wantsplode;
		if (!level[jumpman_s].falloutMessage.empty())
			addFloater(level[jumpman_s].falloutMessage);
	}
}

cpVect lastClicked;

void clickLastResortHandle(double in_x, double in_y, int button, bool justDragging) {
			cpVect click = cpv(in_x, in_y);
			
	switch (edit_mode) {
	
		case EPlayground: { // This is horribly messy and breaks all my abstraction and *I* *DON'T* *CARE*!
			  cpVect seg_verts[] = {
				cpv(-18,-18),
				cpv(-18, 18),
				cpv( 18, 18),
				cpv( 18,-18),
			  };

			if (doingEnding)
				button = 2;

			if (button == 1) {			
				if (justDragging) { // I no longer remember what this does?!
					if (fabs(click.x-lastClicked.x) > 18) {
						click.y = lastClicked.y;
					} else if (fabs(click.y-lastClicked.y) > 18) {
						click.x = lastClicked.x;
					}
					else return;
				}
				lastClicked = click;			
				
				click = rot(cpv(in_x, in_y), cam_track);
				click.x = -click.x; // Why?
				
				for(int c = 0; c < 4; c++)
					seg_verts[c] = cpvrotate(seg_verts[c], cpvforangle(roten));
					  
				cpShape *seg = cpPolyShapeNew(level[0].staticBody, 4, seg_verts, click);
				seg->u = 0.1;
				seg->e = 1.0;
			
				seg->collision_type = C_FLOOR;
				cpSpaceAddStaticShape(level[0].space, seg);
			} else { // REDUNDANT! Well I guess none of this code survives anyway...
					if (justDragging) return;
			
					cpBody *body = cpBodyNew(INFINITY, INFINITY);
					click.x = -click.x; // Why?
				
					body->p = click;
					body->v = cpvrotate( pinfo[C_MARCH]->constv, cpvforangle(roten));
					//cpBodySetAngle(body, r*M_PI/4); // Don't
					
					cpShape *shape;
					if (button == 3) { // Awkward!
						shape = cpCircleShapeNew(body, 36.0, cpvzero);
						shape->e = 0.80;
						shape->collision_type = C_BALL;
					} else {
						shape = cpPolyShapeNew(body, 4, seg_verts, cpvzero);
						shape->collision_type = C_MARCH;
						shape->u = 0;
						if (doingEnding) { // Delete me
							shape->collision_type = C_BIRD;
							shape->layers = 0;
							body->v = cpv(-400.0,25.0);
						}
					}	  
					
					plateinfo *plate = pinfo[shape->collision_type];
					if (plate->defaultdata)
						body->data = plate->defaultdata->clone(); 
					
					cpBodySetMass(body, 5);
					cpSpaceAddBody(level[0].space, body);
					cpSpaceAddShape(level[0].space, shape);

			}

		} break;
		
		case EWalls: {
			bool rewrite = false;
			unsigned int write;
		
			if (button == 1) {
				rewrite = true;
				write = chosenTool;
			} else if (button == 2) {
				if (justDragging) {
					double offsx = click.x - lastClicked.x,
						offsy = click.y - lastClicked.y;
					offsx *= 2.0/3;
					offsy *= 2.0/3;
					scan_x += offsx;
					scan_y += offsy;
					click.x -= offsx;
					click.y -= offsy;
					
//					ERR("[scan %lf, %lf]\n", scan_x, scan_y);
				}
				lastClicked = click;
			} else if (button == 3) {
				rewrite = true;
				write = 0;
			} else if (button == 4) {
				scan_r *= 1.1;
				rePerspective = true;
			} else if (button == 5) {
				scan_r /= 1.1;
				rePerspective = true;
			}
			
			if (rewrite) {
				double fcurrent = roten / (M_PI/4);
				int current = (fabs(fcurrent) + 0.5); // WTF				
				
				//ERR("CURRENT %lf %d %d\n", fcurrent, current, current%2);
				int irot = ((fcurrent < 0 ? 8-current : current) / 2) % 4;
				current %= 2;
				current += (editLayer * 2);
		
				if (current >= editSlice.size())
					return; // Don't proceed if there is no angle
		
				double _x = -(click.x-(editSlice[current]->width%2?27:0)-scan_x/2)/54, _y = -(click.y-(editSlice[current]->height%2?27:0)-scan_y/2)/54;
				int x = _x + editSlice[current]->width/2, y = _y + editSlice[current]->height/2;
				bool wrote = false, resized = false;
				
				int t;
				switch(irot) { // This coordinate system is a disaster
					case 0: break;
					case 1: t = x; x = y; y = editSlice[current]->width-t-1; break;
					case 2: x = editSlice[current]->width-x-1; y = editSlice[current]->height-y-1; break;
					case 3: t = x; x = editSlice[current]->height-y-1; y = t; break;
				}
				
//				ERR("CUR+IROT %lf %d %d\n", fcurrent, current, irot);
				
//				ERR("%lf,%lf -> %d, %d [scan %lf, %lf -> %lf, %lf -> %lf, %lf]\n", -click.x, -click.y, x, y, scan_x, scan_y, _x, _y, -(click.x-scan_x), -(click.y-scan_y));
				
				if (write == C_SCROLL) {					
					if (justDragging) {
						if (!dragPoison) {
							int distX = x-lineStartX;
							int distY = y-lineStartY;
							if (distX || distY) {
								unsigned int temp[editSlice[current]->width];
								if (distX) {
									for(int cy = 0; cy < editSlice[current]->width; cy++) {
										for(int cx = 0; cx < editSlice[current]->width; cx++)
											temp[cx] = editSlice[current]->pixel[cx][cy];
										for(int cx = 0; cx < editSlice[current]->width; cx++) {
											int nx = cx + distX;
											nx %= editSlice[current]->width;
											if (nx < 0) nx += editSlice[current]->width;
											editSlice[current]->pixel[nx][cy] = temp[cx];
										}
									}
								}
								if (distY) {
									for(int cx = 0; cx < editSlice[current]->width; cx++) {
										for(int cy = 0; cy < editSlice[current]->width; cy++)
											temp[cy] = editSlice[current]->pixel[cx][cy];
										for(int cy = 0; cy < editSlice[current]->width; cy++) {
											int ny = cy + distY;
											ny %= editSlice[current]->width;
											if (ny < 0) ny += editSlice[current]->width;
											editSlice[current]->pixel[cx][ny] = temp[cy];
										}
									}
								}
								
								wrote = true;
							}
						}
					} else { // I.E. if not just dragging, this is a new click
						dragPoison = !level[0].repeat && !( // DRY DRY DRY DRY!!
							x >= 0 && y >= 0 && 
							x < editSlice[current]->width && y < editSlice[current]->height );
					}
					
					lineStartX = x; lineStartY = y;
				} else if (write == C_RESIZE) {
					int w = int(fabs(_x)+1)*2, h = int(fabs(_y)+1)*2;
					if (w < 2) w = 2; if (h < 2) h = 2;
					if (irot % 2) { t = w; w = h; h = t; }
//					ERR("RESIZE %lf,%lf -> %lf,%lf (= %d, %d)\n", (double)_x, (double)_y, (fabs(_x)+1), (fabs(_y)+1), (int)w, (int)h);
					
					if (w > h) h = w;
					if (h > w) w = h;
					
					slice *newslice = new slice(), *oldslice = editSlice[current];
					newslice->init(w, h);
					for(int nx = 0; nx < w; nx++) {
						for(int ny = 0; ny < h; ny++) {
							uint32_t &pixel = newslice->pixel[nx][ny];
							int ox = nx - w/2 + oldslice->width/2,
								oy = ny - h/2 + oldslice->height/2;
							if (ox >= 0 && oy >= 0 && ox < oldslice->width && oy < oldslice->height) {
								pixel = oldslice->pixel[ox][oy]; 
							} else {
								pixel = 0;
							}					
						}
					}
					editSlice[current] = newslice;
					delete oldslice;
					
					wrote = true;
					resized = true;
				} else if (write == C_RESIZE2) { // ...DRY?!?... Oh, fuck it
					int w = int(fabs(_x)+(_x<0?1:0))*2+1, h = int(fabs(_y)+(_y<0?1:0))*2+1;
					if (w < 1) w = 1; if (h < 1) h = 1;
					if (irot % 2) { t = w; w = h; h = t; }
//					ERR("RESIZE2 %lf,%lf -> %d,%d (= %d, %d)\n", (double)_x, (double)_y, int(fabs(_x)), int(fabs(_y)), (int)w, (int)h);
					
					if (w > h) h = w;
					if (h > w) w = h;
					
					slice *newslice = new slice(), *oldslice = editSlice[current];
					newslice->init(w, h);
					for(int nx = 0; nx < w; nx++) {
						for(int ny = 0; ny < h; ny++) {
							uint32_t &pixel = newslice->pixel[nx][ny];
							int ox = nx - w/2 + oldslice->width/2,
								oy = ny - h/2 + oldslice->height/2;
							if (ox >= 0 && oy >= 0 && ox < oldslice->width && oy < oldslice->height) {
								pixel = oldslice->pixel[ox][oy]; 
							} else {
								pixel = 0;
							}					
						}
					}
					editSlice[current] = newslice;
					delete oldslice;
					
					wrote = true;
					resized = true;
				} else if (x >= 0 && y >= 0 && 
					x < editSlice[current]->width && y < editSlice[current]->height) {
					if (chosenTool == C_LINE) {
						if (!justDragging) {
							if (write)
								write = C_FLOOR;
							if (haveLineStart) {
								int distx = iabs(lineStartX-x), disty = iabs(lineStartY-y);
								bool xIsLonger = distx >= disty;
								int distz = (xIsLonger ? distx : disty);
								int dz = (xIsLonger ? lineStartX<x : lineStartY<y) ? 1 : -1;
								for(int z = 0; z <= distz; z++) {
									if (xIsLonger) {
										editSlice[current]->pixel[lineStartX+z*dz][lineStartY] = write;
									} else {
										editSlice[current]->pixel[lineStartX][lineStartY+z*dz] = write;
									}
								}
							
								wrote = true;
								haveLineStart = false;
							} else {
								lineStartX = x; lineStartY = y;
								haveLineStart = true;
							}		
						}	
					} else if (editSlice[current]->pixel[x][y] != write) {
						if (write == C_ENTRY) { // Special behavior if we've moved Jumpman
							for(int px = 0; px < editSlice[current]->width; px++) {
								for(int py = 0; py < editSlice[current]->height; py++) {
									uint32_t &pixel = editSlice[current]->pixel[px][py];
									if (pixel == C_ENTRY) {
										pixel = 0;
									}
								}
							}
							chassis->p.x = 36*(x - editSlice[current]->width/2); chassis->p.y = -36*(y - editSlice[current]->height/2); // Center camera. TODO: DRY?
						}
						
						unsigned int realwrite = write; 
						
						if (realwrite && !C_SYMTYPE(realwrite)) { // Pack in rotation information
							realwrite &= C_MASK;
							realwrite |= ( (irot & 3) ^ C_META );
						}
						
						editSlice[current]->pixel[x][y] = realwrite;
					
						wrote = true;
					}
				}
				
				if (resized && 0==current) {
					level[0].base_width = 36*(editSlice[current]->width);
					level[0].repeat_every = 36*(editSlice[current]->width);
				}
				if (wrote) {
					cpVect p = chassis->p, v = chassis->v; // CHEAP CHEAP CHEAP CHEAP!

					repaintEditSlice();
					reentryEdit();
					
					chassis->p = p; chassis->v = v;
					
					onEsc = WSave;
				}
				
			}
		} break;
		
		case EAngle: {
			if (!justDragging) {
				cpFloat angle = cpvtoangle(click) + M_PI;
				int idx = 7 - (int(fabs(angle / (M_PI/4)) + 0.5 + 5) % 8);
//				ERR("%lf, %lf, %lf, %d\n", (double)click.x, (double)click.y, (double)angle, idx);
				int bit = (1 << idx);
				
				if (button == 1) {
					level[jumpman_s].rots ^= bit;
					level[jumpman_s].dontrot &= ~bit;
				} else if (button == 3) {
					level[jumpman_s].rots &= ~bit;
					level[jumpman_s].dontrot ^= (1 << idx);
				}
			}
		} break;
		
		case ECoordinates: { // Debug tool
			if (!justDragging) {
				char filename2[FILENAMESIZE];
				snprintf(filename2, FILENAMESIZE, "(%lf, %lf)", (double)-click.x/433, (double)click.y/433);
				ERR("%s\n", filename2);

				endingYell(filename2, -click.x/433, click.y/433); 
			}
		} break;
		
		default:break;
	}
}

void repaintEditSlice() { // whatever, i don't know
	cpSpaceFree(level[0].space);
	addSpaceTo(level[0]);
	setupCollisions(level[0]);

	for(int c = 0; c < editSlice.size(); c++) {
		slice *temp = editSlice[c]->clone();
		if (0 == c)
			temp->border(&level[0]);
		temp->construct();
		
		loadLevel(&level[0], *temp, c%2, c/2);
		
		delete temp;
	}
}

void reentryEdit() {
	if (!paused) {
		jumpman_reset();
	}	
}


bool haveTimerData = false, wantTimerData = false, censorTimerData = false;
int timerStartsAt = 0, timerEndsAt = 0, timerPausedAt = 0, timerLives = 0;
int subTimerStartsAt = 0, subTimerLives = 0;
void startTimer()
{
	haveTimerData = false; wantTimerData = true;
	timerStartsAt = SDL_GetTicks();
	timerLives = 0;
	subTimerStartsAt = SDL_GetTicks();
	subTimerLives = 0;
}
void completeTimer()
{
	if (wantTimerData) {
		haveTimerData = true; wantTimerData = false;
		timerEndsAt = SDL_GetTicks();
	}
}
void pause(bool _paused)
{
	// Elaborate tricks so that time spent paused doesn't hurt your play time (although there's a 100 ms penalty per pause)
	if (wantTimerData) {
		if (_paused) {
			timerPausedAt = SDL_GetTicks();
		} else {
			int pauseGap = SDL_GetTicks();
			pauseGap -= timerPausedAt;
			
			ERR("UNPAUSE from %d to %d diff %d\n", timerStartsAt, timerStartsAt + pauseGap - 100, pauseGap - 100);

			timerStartsAt = timerStartsAt + pauseGap - 100; 
			subTimerStartsAt = subTimerStartsAt + pauseGap - 100;
		}
	}
	paused = _paused;
}
void timer_flag_rollover()
{
	if (wantTimerData && currentScoreKey.size()) {
		pair<scoreinfo,scoreinfo> &s = scores[currentScoreKey];
		int now = SDL_GetTicks();
		bool highscored = false;
		bool firstscore = false;
		
		int ourtime = now - subTimerStartsAt;
		if (!ourtime)
			ourtime++; // WTF
		int theirtime;

ERR(" ZZZZ new: %d, %d scores [%d] flag %d\n", ourtime, subTimerLives, scores.size(), jumpman_flag);
		
		ensurecapacity(s, jumpman_flag+1);
		
		theirtime = s.first.time[jumpman_flag];
		if (!theirtime)
			firstscore = true;
		if (!theirtime || theirtime > ourtime
			|| (theirtime == ourtime && s.first.deaths[jumpman_flag] > subTimerLives)) {
			s.first.time[jumpman_flag] = ourtime;
			s.first.deaths[jumpman_flag] = subTimerLives;
			highscored = true;
		}
		
		theirtime = s.second.time[jumpman_flag];
		if (!theirtime || s.second.deaths[jumpman_flag] > subTimerLives
			|| (s.second.deaths[jumpman_flag] == subTimerLives && theirtime > ourtime)) {
			s.second.time[jumpman_flag] = ourtime;
			s.second.deaths[jumpman_flag] = subTimerLives;
			highscored = true;
		}		
		jumpman_flag++;
		
		if (flags.size() > 1) {
			char filename[FILENAMESIZE];	
			int seconds = ourtime / 1000;
			int minutes = seconds / 60;
			seconds %= 60;
			if (minutes)
				snprintf(filename, FILENAMESIZE, "Finished path %d in %d minutes, %d seconds; died %d times%s", jumpman_flag, minutes, seconds, subTimerLives, firstscore?". Progress saved":(highscored?". New high score":""));
			else
				snprintf(filename, FILENAMESIZE, "Finished path %d in %d seconds; died %d times%s", jumpman_flag, seconds, subTimerLives, firstscore?". Progress saved":(highscored?". New high score":""));

			addFloater(filename);
		}
	}
	subTimerStartsAt = SDL_GetTicks();
	subTimerLives = 0;
}

// "Ending" follows
#define ENDPANEL 0

#if ENDPANEL
extern ContainerBase *container[5];
WheelControl *endwheels[2][4] = {NULL, NULL, NULL, NULL};
#endif

#define BPM 120
#define SPAN ((44100*60)/BPM)
int modeStartsAt = 0;
int modeEndsAt = 0;

struct endmessage {
	int at;
	const char *msg;
	double x, y;
};
#define EMSGS 16
endmessage emsg[EMSGS] = {
{SPAN*16,		""		,	1,	-0.5},
{SPAN*20,	""		,	1,	-0.7},
{SPAN*32,	"Jumpman",	0.75,	0.35},
{SPAN*40,	"Created by mcc",	-0.75,	-0.35},
{SPAN*48,	""		,	1,	-0.7},
{SPAN*48,	"Featuring art by",	0.18+0.15,	0.58+0.1},
{SPAN*50,	""		,	1,	-0.7},
{SPAN*50,	"Kyou",		0.28+0.15,	0.43+0.1},
{SPAN*52,	""		,	1,	-0.7},
{SPAN*52,	"Eyes5",	0.38+0.15,	0.28+0.1},
{SPAN*62,	"'Nu Sans' font by Marty Pfeiffer",	0.26,	0.63},
{SPAN*64,	""		,	1,	-0.7},
{SPAN*72,	"'Chipmunk' physics library by Scott Lembcke",	0.26,	0.43},
{SPAN*83,	"Thanks to Chris, Diana, T.R. and platformers.net",	0.28,	0.58},
{SPAN*84,	"for support and feedback",	0.38,	0.43},
{SPAN*96,	"www.runhello.com",	0.8,	-0.7},
};

void birdHandler(void *ptr, void *) {
	cpShape *shape = (cpShape*)ptr;
	
	if (shape->collision_type == C_BIRD) {
		cpVect force = {((shape->body->v.x>0?-1:1)-0.1)*25*shape->body->m,0};
		cpBodyApplyForce(shape->body, force, cpv(0,0));		
		if (shape->body->p.x < -1000.0/2/aspect) {
			cpBodyResetForces(shape->body);
			shape->body->v = cpv(-400.0,25.0);
			shape->body->p = cpv(1000.0/2/aspect, ((double)random()/RANDOM_MAX -0.5)*750.0); // All these hardcoded numbers are probably wrong :(
		}
	}
}

void goBlue();

void endingTickMainThread() {
	cpSpaceHashEach(level[0].space->activeShapes, &birdHandler, NULL);
	while (endingAtMsg < EMSGS && emsg[endingAtMsg].at <= endingAt) {
		if (emsg[endingAtMsg].msg[0])
			endingYell(emsg[endingAtMsg].msg, emsg[endingAtMsg].x, emsg[endingAtMsg].y, endingAtMsg+1 < EMSGS ? 1 : 2);
		else {
			  cpVect seg_verts[] = {
				cpv(-18,-18),
				cpv(-18, 18),
				cpv( 18, 18),
				cpv( 18,-18),
			  };
			cpBody *body = cpBodyNew(INFINITY, INFINITY); // Code duplication, again, from the playground code
			cpShape *shape = cpPolyShapeNew(body, 4, seg_verts, cpvzero);
			shape->collision_type = C_BIRD;
			shape->layers = 0;
			shape->u = 0;
			body->v = cpv(-400.0,25.0);
			body->p = cpv(emsg[endingAtMsg].x*1000.0/2/aspect, emsg[endingAtMsg].y*750.0/2); // All these hardcoded numbers are probably wrong :(
			plateinfo *plate = pinfo[shape->collision_type];
			if (plate->defaultdata)
				body->data = plate->defaultdata->clone(); 
			cpBodySetMass(body, 5);
			cpSpaceAddBody(level[0].space, body);
			cpSpaceAddShape(level[0].space, shape);
		}
		endingAtMsg++;
	}
	goBlue();
}

void endingTick()
{
	if (endingAt && modeEndsAt && (endingAt - modeStartsAt == modeEndsAt)) {
		endingAtMode++;
		modeStartsAt = endingAt;
	}
	int at = endingAt - modeStartsAt;
	
#if ENDPANEL
	if (endingAt == 0) {
		container[CMIDL] = new ContainerBase(uiSpace, CMIDL);
		container[CMIDR] = new ContainerBase(uiSpace, CMIDR);
		for(int c = 0; c < 4; c++) {
			endwheels[0][c] = new WheelControl(0, -60, 10, 1);
			container[CMIDL]->add(endwheels[0][c]);
			endwheels[1][c] = new WheelControl(0, -60, 10, 1);
			container[CMIDR]->add(endwheels[1][c]);
		}
		container[CMIDL]->commit();
		container[CMIDR]->commit();
	}
#endif
	
	switch (endingAtMode) {
		case 0: { // BOUNCE... BOUNCE... BOUNCE... BOUNCE
			if (0 == at) {
				sland.reset();
				modeEndsAt = SPAN * 8;
			}
		} break;
		
/*		case 1: { // WWAWAWAWAWAWAW plain
			if (0 == at) {
				sland.max = 5000000;
				sland.w = 400;
				sland.amp = 0.05;				
				modeEndsAt = 0;
				sland.reset();
			}
			
			sland.amp = (sin( double(at)/SPAN * M_PI * 4 ) + 1) * 0.025;
		} break; */
		case 1: // disco tick -- alone
			if (0 == at) {
				modeEndsAt = SPAN*8;
				slick.max = 1750;
				slick.w = 1;
				slick.amp = 0.075;
			}
			if (0 == at % (SPAN/4)) {
				slick.reset();
			}
			break;
		
		/*case 5: case 3: { // WWAWAWAWAWAWAW wah
			if (0 == at) {
				sjump.max = 5000000;
				sjump.w = 400;
				sjump.amp = 0.05;				
				modeEndsAt = SPAN*8;
				sland.reset();
			}
			
			sjump.amp = sin( double(at) / 44100 * M_PI * 55 * 2 ) 
			 * (sin( double(at)/SPAN * M_PI * 4 ) + 1.5)
			 * 1;
			if (sjump.amp > 0.5)
				sjump.amp = 0.5;
			if (sjump.amp < -0.5)
				sjump.amp = -0.5;
			sjump.amp *= 0.75;
			sjump.ticks = sjump.max;
		} //break;*/
		
		
		case 8: case 7: case 6: case 4: case 2: case 3: case 5: { // disco tick
			if (0 == at) {
				sland.max = 5000;
				sland.w = 400;
				sland.amp = 0.05;
				
				slick.max = 1750;
				slick.w = 1;
				slick.amp = 0.075;
				
				shatter.max = 30000;
				shatter.w = 300;
				shatter.amp = 0.1;
				
				sbell.amp = 0.05;
				sbell.w = 200;
				sbell.max = 30000;
				
				modeEndsAt = 0;
				sland.reset();
				if (endingAtMode == 3)
					modeEndsAt = SPAN*32;
				else if (endingAtMode == 9)
					modeEndsAt = 0;
				else
					modeEndsAt = SPAN*16;
			}
			if (endingAtMode < 8 && 0 == at % (SPAN/4)) {
				slick.reset();
			}
			
			int dat = at;
			if (endingAtMode == 4)
				dat *= 2;
				
			if (endingAtMode < 7 && 0 == dat % (SPAN*2) && !(endingAtMode==2 && dat/(SPAN*2)==7)) {
				int w = dat / (SPAN * 2);
				if (w%2) {
					shatter.max = 30000;
					shatter.w = 15;
					shatter.amp = 0.1;
				} else {
					shatter.max = 7500;
					shatter.w = 150;
					shatter.amp = 0.2;
				}
				shatter.reset();
			}
			static int offset = 0;
			if (endingAtMode >= 3) {			
				if (0 == at % (SPAN/6)) {
					int i = at / (SPAN / 6);
					int wsi = 0;
					bool up = false;
					
					if ((i / 48) % 2) {
						wsi++;
						up = true;
						//offset = 2;
					}
						//else offset = 0;
						
#if ENDPANEL
					int w = floor(endwheels[up?1:0][i % 3]->is);
#else
					int ws[2][3] = {{0, -4, -12}, {-14,-4,-7}};
					int w = ws[wsi][i%3];
#endif
					
					sbell.w = 100;
//						if ((i/16)%16 == 14)
//							offset -= 2;
//						w += offset;
					double s = pow(pow(2, 1/12.0), w);
					sbell.w *= s;
					//ERR("w: %d s: %lf w: %d\n", w, s, sbell.w);
					if (0 == i%4) sbell.reset();
				}
			}
			
			if (endingAtMode < 6 && 0 == at % (SPAN/8) && !(endingAtMode==2 && at/(SPAN*4)==3)) {
				int w = (at / (SPAN/8)) % 32;
//				if (w == 8 || w == 12 || w == 24 || w == 26 || w == 28) {
//				if (w == 6 || w == 10	|| w == 22 || w == 27 || w == 26) {
				if (w == 20 || w == 28	|| w == 6 || w == 12 || w == 10) {
					sland.amp = 0.1;
					sland.w = 400;
					if (offset) {
						double s = pow(pow(2, 1/12.0), offset);
						sland.w *= s;
					}
					sland.reset();
				}
			}
		} break;
		case 9: {
			if (0 == at) {
				modeEndsAt = 0;
			}
		} break;
	}
	
	endingAt++;
	endingAtBar = endingAt / SPAN;
}
