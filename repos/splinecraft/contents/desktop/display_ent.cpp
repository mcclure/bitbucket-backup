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

ent *global_ent = new display_ent();

// Infrastructure

static uint32_t display_code_generator = D_MAX + 1;
display_code display_code_unique() {
	return (display_code)display_code_generator++;
}

extern double aspect;

drawing::drawing() : w(1/aspect),h(1) {}

drawing::~drawing() {
	for(quad_db_iter i = data.begin(); i != data.end(); i++)
		delete i->second;
}

pile &drawing::get_pile(display_code code, elementoid *_elementDrawer, bool _useThree, bool _useTexture, bool _useColor, GLint _texture, unsigned int _baseColor) {
	pile *&result = data[code];
	if (!result) {
		result = new pile(_elementDrawer, _useThree, _useTexture, _useColor, _texture, _baseColor);
	}
	return *result;
}

quadpile &drawing::get_quadpile(display_code code, bool _useTexture, bool _useColor, GLint _texture, unsigned int _baseColor) {
	pile *&result = data[code];
	if (!result) {
		result = new quadpile(_useTexture, _useColor, _texture, _baseColor);
	}
	return *(quadpile *)result;
}

quadpile3 &drawing::get_quadpile3(display_code code, bool _useTexture, bool _useColor, GLint _texture, unsigned int _baseColor) {
	pile *&result = data[code];
	if (!result) {
		result = new quadpile3(_useTexture, _useColor, _texture, _baseColor);
	}
	return *(quadpile3 *)result;
}


void drawing::dupe(drawing *target) {
	w = target->w; h = target->h;
}

void drawing::size(texture_slice *target) {
	w = target->sub_width() / float(target->sub_height());
	h = 1;
}

void drawing::execute() {
	for(quad_db_iter i = data.begin(); i != data.end(); i++)
		i->second->draw();
}

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
	drawing d;
	
	ent::display(&d);
	
	d.execute();
	GLERR("display");
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