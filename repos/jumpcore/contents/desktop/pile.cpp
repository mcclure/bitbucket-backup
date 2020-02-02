/*
 *  pile.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 12/2/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "display.h"
#include "glCommon.h"
#include "glCommonMatrix.h"

// TODO: move drawing into pile so this is not needed
#include "pile.h"

// Drawing

drawing::drawing(float width, float height) { size(width, height); }
drawing::drawing(texture_slice *target) { size(target); }
drawing::drawing(drawing *target) { dupe(target); }

drawing::~drawing() {
	for(quad_db_iter i = data.begin(); i != data.end(); i++)
		if (!i->second->immortal)
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

tilepile &drawing::get_tilepile(texture_slice *s, int x, int y, cpVect _size, cpVect _offset, bool xyIsPixels, bool _useColor, unsigned int _baseColor) {
	tilepile *result = new tilepile(s, x, y, _size, _offset, xyIsPixels, _useColor, _baseColor);
	insert_pile(display_code_unique(), result);
	return *result;
}

void drawing::size(float _w, float _h) {
	float coef = YFLIP ? -1 : 1;
	w = _w; h = _h;
	mvp = glm::ortho( -w, w, coef*-h, coef*h, -1.0f, 1.0f);
}

void drawing::size(texture_slice *target) {
	size(target->sub_width() / float(target->sub_height()), 1);
}

void drawing::dupe(drawing *target) {
	size(target->w, target->h);
}

void drawing::execute() {
	for(quad_db_iter i = data.begin(); i != data.end(); i++)
		i->second->draw(this);
}

void drawing::matrix_upload() { // TODO: Make this like a jcMatrixRawSet() or something.
	if (gl2)
		glUniformMatrix4fv( p->uniforms[s_mvp_matrix], 1, GL_FALSE, glm::value_ptr(mvp) );
	else {
		matrix_jcupload();
		void mesa_sync();
		mesa_sync();
	}
}

void drawing::matrix_jcupload() {
	jcMatrixGlmIn(mvp);
}

// Auto-growing vertex arrays:

// Vertexes, then color, then texture
#define VERT_STRIDE 2
#define COLOR_STRIDE 1
#define TEX_STRIDE 2

static uint32_t display_code_generator = D_MAX + 1;
display_code display_code_unique() {
	return (display_code)display_code_generator++;
}

// Assumes texes is valid if useTexture
void pile::push(float x, float y, float z, unsigned int color, float *texes) {
    push_back(x);
    push_back(y);
	if (useThree)
		push_back(z);
    if (useColor)
		push_back(*((float *)&color));
    if (useTexture) {
        push_back(texes[0]);
        push_back(texes[1]);
    }
}

// Returns size of a single vertex in units sizeof(float)
int pile::stride() const {
	return 2 + useThree + useColor + useTexture*2;
}

int pile::vertices() const {
	return size() / stride();
}

float full_texes[8] = { 0, 1, 0, 0, 1, 0, 1, 1 };
#define TEX_ENSURE(name) if (useTexture && !name) name = full_texes;

void quadpile::push4(cpVect *vertices, unsigned int color, float *texverts, float z) {
	TEX_ENSURE(texverts);
		
	for(int c = 0; c < 4; c++) {
		cpVect &v = vertices[c];
		push(v.x, v.y, z, color, texverts + (c*2));
	}			
}

void quadpile::push4(cpVect *vertices, unsigned int color, texture_slice *s, float z) {
	float texes[8] = { s->coords[0], s->coords[3], s->coords[0], s->coords[1], s->coords[2], s->coords[1], s->coords[2], s->coords[3] };
    push4(vertices, color, texes, z);
}

void quadpile::push4(float x1, float y1, float x2, float y2, unsigned int color, float *texverts, float z) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
	push4(vertices, color, texverts, z);
}

void quadpile::push4(float x1, float y1, float x2, float y2, unsigned int color, texture_slice *s, float z) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
    push4(vertices, color, s, z);
}

void quadpile::push4(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float *texverts, float z) {
    push4(x1,y1,x2,y2,packColor(r, g, b, a),texverts, z);
}

void quadpile::push4(float x1, float y1, float x2, float y2, float r, float g, float b, float a, texture_slice *s, float z) {
    push4(x1,y1,x2,y2,packColor(r, g, b, a),s, z);
}

// Someday, I am going to have to test to see if the texture coordinates here are correct.
// And on that day, I will be very sad.

void quadpile3::xysheet(cpVect a, cpVect b, cpFloat z1, cpFloat z2, unsigned int color, float *texes) {
	TEX_ENSURE(texes);
	push(a.x, a.y, z1, color, texes+0);
	push(a.x, a.y, z2, color, texes+2);
	push(b.x, b.y, z2, color, texes+4);
	push(b.x, b.y, z1, color, texes+6);
}

void quadpile3::xzsheet(cpVect a, cpVect b, cpFloat y1, cpFloat y2, unsigned int color, float *texes) {
	TEX_ENSURE(texes);
	push(a.x, y1, a.y, color, texes+0);
	push(a.x, y2, a.y, color, texes+2);
	push(b.x, y2, b.y, color, texes+4);
	push(b.x, y1, b.y, color, texes+6);
}

void quadpile3::yzsheet(cpVect a, cpVect b, cpFloat x1, cpFloat x2, unsigned int color, float *texes) {
	TEX_ENSURE(texes);
	push(x1, a.x, a.y, color, texes+0);
	push(x2, a.x, a.y, color, texes+2);
	push(x2, b.x, b.y, color, texes+4);
	push(x1, b.x, b.y, color, texes+6);
}


void quadpile3::xyplane(cpVect a, cpVect b, cpFloat z, unsigned int color, float *texes) {
	TEX_ENSURE(texes);
	push(a.x, a.y, z, color, texes+0);
	push(a.x, b.y, z, color, texes+2);
	push(b.x, b.y, z, color, texes+4);
	push(b.x, a.y, z, color, texes+6);
}

void quadpile3::xzplane(cpVect a, cpVect b, cpFloat y, unsigned int color, float *texes) {
	TEX_ENSURE(texes);
	push(a.x, y, a.y, color, texes+0);
	push(a.x, y, b.y, color, texes+2);
	push(b.x, y, b.y, color, texes+4);
	push(b.x, y, a.y, color, texes+6);
}

void quadpile3::yzplane(cpVect a, cpVect b, cpFloat x, unsigned int color, float *texes) {
	TEX_ENSURE(texes);
	push(x, a.x, a.y, color, texes+0);
	push(x, a.x, b.y, color, texes+2);
	push(x, b.x, b.y, color, texes+4);
	push(x, b.x, a.y, color, texes+6);
}

tilepile::tilepile(texture_slice *s, int x, int y, cpVect _size, cpVect _offset, bool xyIsPixels, bool _useColor, unsigned int _baseColor)
	: quadpile(true, _useColor, s->texture, _baseColor), size(_size), offset(_offset)
{
	if (!xyIsPixels) {
		w = x;
		x = s->twidth/x;
		y = s->twidth/y;
	} else {
		w = s->twidth/x;
	}
	pixel = cpv(float(x)/s->twidth, float(y)/s->twidth);
}

void tilepile::tile(int ix, int iy, int x, int y, cpVect _offset, unsigned int color) {
	cpVect v1 = cpvadd( cpvscale( cpv(x, y), size ), cpvadd(offset, _offset));
	cpVect v2 = cpvadd(v1, size);
	cpVect from = cpvscale(cpv(ix, iy), pixel);
	cpVect to =   cpvscale(cpv(ix+1, iy+1), pixel);
	float clip_texes[8] = { from.x, to.y, from.x, from.y, to.x, from.y, to.x, to.y };
	push4(v1.x, v1.y, v2.x, v2.y, color, clip_texes, z);
}

void tilepile::tile(int i, int x, int y, cpVect _offset, unsigned int color) {
	tile(i%w, i/w, x, y, _offset, color);
}

void state_common::set(pile &q, drawing *d) {
	d->matrix_upload(); // TODO: Let the pile have a matrix_source
}

state_basic *state_basic::single = new state_basic();
void state_basic::set(pile &q, drawing *d) {
	States(q.useTexture, q.useColor);
	state_common::set(q, d);
}

element_plain *element_plain::points = new element_plain(GL_POINTS);
element_plain *element_plain::triangles = new element_plain(GL_TRIANGLES);
void element_plain::draw(pile &q) {
	glDrawArrays(mode, 0, q.vertices());
}

element_quad *element_quad::single = new element_quad();
void element_quad::draw(pile &q) {
	int pushed = q.vertices()/4; // This many quads
	megaIndexEnsure(pushed);

	glDrawElements(GL_TRIANGLES, pushed*6, GL_UNSIGNED_SHORT, &element_quad::megaIndex[0]); // *6 because enough indices for two triangles
}

void pile::draw(drawing *d) {
	stateSetter->set(*this, d);
	
	int vertPlus = VERT_STRIDE + useThree;
	int stride = vertPlus;
	int texPlus = 0;
	if (useColor) {
		stride += COLOR_STRIDE; texPlus += COLOR_STRIDE;
	} else {
		jcColor4ubv((GLubyte*)&baseColor);
	}
	if (useTexture) {
		stride += TEX_STRIDE;
		glBindTexture(GL_TEXTURE_2D, texture);
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	jcVertexPointer(vertPlus, GL_FLOAT, stride*sizeof(float), &(*this)[0]);
	if (useColor) {
		jcColorPointer(4, GL_UNSIGNED_BYTE, stride*sizeof(float), &(*this)[0]+vertPlus);
	}
	if (useTexture) {
		jcTexCoordPointer(2, GL_FLOAT, stride*sizeof(float), &(*this)[0]+vertPlus+texPlus);
		stride += TEX_STRIDE;
	}
	
	elementDrawer->draw(*this);	
}

vector<unsigned short> element_quad::megaIndex;
void element_quad::megaIndexEnsure(int count) {
	int current = megaIndex.size()/6;
	for(int c = current; c < count; c++) {
		megaIndex.push_back(c*4);
		megaIndex.push_back(c*4+3);
		megaIndex.push_back(c*4+1);
		megaIndex.push_back(c*4+1);
		megaIndex.push_back(c*4+3);
		megaIndex.push_back(c*4+2);
	}
}

vector<unsigned short> element_quad_wire::megaIndex;
element_quad_wire *element_quad_wire::single = new element_quad_wire();
void element_quad_wire::megaIndexEnsure(int count) {
	int current = megaIndex.size()/8;
	for(int c = current; c < count; c++) {
		megaIndex.push_back(c*4);
		megaIndex.push_back(c*4+1);
		megaIndex.push_back(c*4+1);
		megaIndex.push_back(c*4+2);
		megaIndex.push_back(c*4+2);
		megaIndex.push_back(c*4+3);
		megaIndex.push_back(c*4+3);
		megaIndex.push_back(c*4);
	}
}

void element_quad_wire::draw(pile &q) {
	int pushed = q.vertices()/4; // This many quads
	megaIndexEnsure(pushed);

	glDrawElements(GL_LINES, pushed*8, GL_UNSIGNED_SHORT, &megaIndex[0]); // *8 because 2 points per side
}

state_blackout *state_blackout::single = new state_blackout();
void state_blackout::set(pile &q, drawing *d) {
	States(q.useTexture, false);
	state_common::set(q, d);
	uint32_t color = BLACK;
	jcColor4ubv((GLubyte*)&color);
}