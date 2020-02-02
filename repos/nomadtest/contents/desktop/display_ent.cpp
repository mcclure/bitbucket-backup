/*
 *  display_ent.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/28/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "display_ent.h"
#include "glCommon.h"
#include "glCommonMatrix.h"

ent *global_ent = new display_ent();

// Infrastructure

texture_source::texture_source(texture_slice *s, bool own) : code(display_code_unique()) {
	texture_load(s, ownTexture);
}
texture_source::texture_source(const char *name) : code(display_code_unique()) {
	texture_load(name);
}
texture_source::~texture_source() {
	if (ownTexture) delete outTexture;
}
void texture_source::texture_load(texture_slice *s, bool own) {
	outTexture = s;
	ownTexture = own;
}
void texture_source::texture_load(const char *name) {
	texture_slice *s = new texture_slice();
	s->load(name);
	texture_load(s,true);
}

// Ents

void lockout::display(drawing *) {
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	jcImmediateColor4f(0.0,0.0,0.0,1.0);
	drawText("Sorry, an OpenGL 2.0 compatible video card is required.", 0, 0.1, 0, true, true);
	drawText("Press esc to quit.", 0, -0.1, 0, true, true);
}

void display_ent::display(drawing *) {
	jcPushMatrix(); // When can I remove these?
	drawing d;
	
	ent::display(&d);
	
	d.execute();
	jcPopMatrix();
	GLERR("display");
}

void eraser::display(drawing *) {
	glClearColor(r,g,b,a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void splatter::display(drawing *d) {
	cpVect from = cpv(-d->w, d->h), to = cpv(d->w, -d->h);
	cpVect verts[4] = RECT(from, to);
	quadpile &into = d->get_quadpile(src->code, true, false, src->outTexture->texture);
	into.stateSetter = stateSetter;
	into.push4(verts, WHITE, src->outTexture);
}

void fixed_splatter::display(drawing *d) {
	cpVect from = cpvadd(cpvscale(size, cpv(-1, 1)), offset), to = cpvadd(cpvscale(size, cpv(1, -1)), offset);
	cpVect verts[4] = RECT(from, to);
	quadpile &into = d->get_quadpile(src->code, true, false, src->outTexture->texture);
	into.stateSetter = stateSetter;
	into.push4(verts, WHITE, src->outTexture);
}

void boxer::display(drawing *d) {
	cpVect from = cpvadd(cpvscale(size, cpv(-1, 1)), offset), to = cpvadd(cpvscale(size, cpv(1, -1)), offset);
	cpVect verts[4] = RECT(from, to);
	quadpile &into = d->get_quadpile(D_P, false, false);
	into.push4(verts);
}