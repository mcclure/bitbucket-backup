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
#include <plaid/audio.h>
#include <plaid/audio/synth.h>
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
			s->pixel[x][y] = (x/(s->width/3) + y/(s->height/3))%2 ? 0xFF00FF00 : 0xFFFF0000 ;
		}
	}
}

void testfill(slice *s, unsigned int color) {
	for(int x = 0; x < s->width; x++) {
		for(int y = 0; y < s->height; y++) {
			s->pixel[x][y] = color;
		}
	}
}

void teststatic(slice *s, float low, float high) {
	for(int x = 0; x < s->width; x++) {
		for(int y = 0; y < s->height; y++) {
			s->pixel[x][y] = packGray( low + (high-low)*random()/float(RANDOM_MAX) );
		}
	}
}