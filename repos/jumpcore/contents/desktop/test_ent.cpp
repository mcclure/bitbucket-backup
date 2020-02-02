/*
 *  test_ent.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/9/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "game.h"
#include "display_ent.h"
#include "test_ent.h"
#include "program.h"
#include "glCommon.h"
#include "test_ent.h"

void testdiamond::display(drawing *d) {
	cpVect v[4] = {cpv(1,0),cpv(0,1),cpv(-1,0),cpv(0,-1)};
	int a = age();
	float r = 0.9*::min(d->w, d->h);
	
	d->matrix_jcupload();
	jcBegin(GL_QUADS);
	for(int c = 0; c < 4; c++) {
		cpVect v2 = cpvrotate(cpvmult(v[c], r), cpvforangle(a/100.0));
		jcImmediateColorWord(packHsv((a*(c+1))%360, 1, 1));
		jcVertex2f(v2.x,v2.y);
	}
	jcEnd();
}

void testsquarefall::display(drawing *d) {
	cpVect v[4] = {cpv(-1,-1),cpv(1,-1),cpv(1,1),cpv(-1,1)};
	float r = 0.9*d->h;
	float off = (fmod(age() / 100.0, 1)-0.5) * -d->h*4;
	
	d->matrix_jcupload();
	jcBegin(GL_QUADS);
	for(int c = 0; c < 4; c++) {
		cpVect v2 = cpvadd( cpvrotate(cpvmult(v[c], r), cpvforangle(0)), cpv(0,off) );
		jcImmediateColorWord(packColor(1,c==1||c==2?0:1,c==1||c==2?0:1));
		jcVertex2f(v2.x,v2.y);
	}
	jcEnd();
}

void testcross(slice *s) {
	for(int x = 0; x < s->width; x++) {
		for(int y = 0; y < s->height; y++) {
			s->pixel(x,y) = (x/(s->width/3) + y/(s->height/3))%2 ? 0xFF00FF00 : 0xFFFF0000 ;
		}
	}
}

void testfill(slice *s, unsigned int color) {
	for(int x = 0; x < s->width; x++) {
		for(int y = 0; y < s->height; y++) {
			s->pixel(x,y) = color;
		}
	}
}

void testcol(slice *s, int x, unsigned int color) {
	for(int y = 0; y < s->height; y++) {
		s->pixel(x,y) = color;
	}
}

void testrow(slice *s, int y, unsigned int color) {
	for(int x = 0; x < s->width; x++) {
		s->pixel(x,y) = color;
	}
}

void teststatic(slice *s, float low, float high) {
	for(int x = 0; x < s->width; x++) {
		for(int y = 0; y < s->height; y++) {
			s->pixel(x,y) = packGray( low + (high-low)*random()/float(RANDOM_MAX) );
		}
	}
}

void slice_copy_scaleup(slice *dst, slice *src, int pixelscale) {
	for(int y = 0; y < src->height; y++) {
		for(int x = 0; x < src->width; x++) {
			uint32_t color = src->pixel(x,y);
			for(int y2 = 0; y2 < pixelscale; y2++) {
				for(int x2 = 0; x2 < pixelscale; x2++) {
					int y3 = y*pixelscale+y2,
					    x3 = x*pixelscale+x2;
					if (dst->contains(x3,y3))
						dst->pixel(x3,y3) = color;
				}
			}
		}
	}
}

// dst is larger
void slice_copy_to_larger(slice *dst, slice *src, int dst_x, int dst_y, int dx, int dy) {
	if (dx < 0) dx = src->width;  dx = ::min(dx, dst->width);
	if (dy < 0) dy = src->height; dy = ::min(dy, dst->height);
	if (dst_x < 0) dst_x = neg_imod(dst_x, dst->width);
	if (dst_y < 0) dst_y = neg_imod(dst_y, dst->height);
	
	for(int y = 0; y < dy; y++) {
		for(int x = 0; x < dx; x++) {
			uint32_t color = src->pixel(x,y);
			if (color)
				dst->pixel((x+dst_x)%dst->width,(y+dst_y)%dst->height) = color;
		}
	}
}

// src is larger
void slice_copy_to_smaller(slice *dst, slice *src, int dst_x, int dst_y, int dx, int dy) {
	int src_x = -dst_x, src_y = -dst_y;

	if (dx < 0) dx = src->width;  dx = ::min(dx, dst->width);
	if (dy < 0) dy = src->height; dy = ::min(dy, dst->height);
	if (src_x < 0) src_x = neg_imod(src_x, src->width);
	if (src_y < 0) src_y = neg_imod(src_y, src->height);
	
	dx = ::min(dx, dst->width);
	dy = ::min(dy, dst->height);
	
	for(int y = 0; y < dy; y++) {
		for(int x = 0; x < dx; x++) {
			uint32_t color = src->pixel((x+src_x)%src->width,(y+src_y)%src->height);
			if (color)
				dst->pixel(x,y) = color;
		}
	}
}