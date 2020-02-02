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

// Auto-growing vertex arrays:

// Vertexes, then color, then texture
#define VERT_STRIDE 2
#define COLOR_STRIDE 1
#define TEX_STRIDE 2

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

float full_texes[8] = { 0, 1, 0, 0, 1, 0, 1, 1 };

void quadpile::push4(cpVect *vertices, unsigned int color, float *texverts) {
	if (useTexture && !texverts)
		texverts = full_texes;
		
	for(int c = 0; c < 4; c++) {
		cpVect &v = vertices[c];
		push(v.x, v.y, 0, color, texverts + (c*2));
	}			
}

void quadpile::push4(cpVect *vertices, unsigned int color, texture_slice *s) {
	float texes[8] = { s->coords[0], s->coords[3], s->coords[0], s->coords[1], s->coords[2], s->coords[1], s->coords[2], s->coords[3] };
    push4(vertices, color, texes);
}

void quadpile::push4(float x1, float y1, float x2, float y2, unsigned int color, float *texverts) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
	push4(vertices, color, texverts);
}

void quadpile::push4(float x1, float y1, float x2, float y2, unsigned int color, texture_slice *s) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
    push4(vertices, color, s);
}

void quadpile::push4(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float *texverts) {
    push4(x1,y1,x2,y2,packColor(r, g, b, a),texverts);
}

void quadpile::push4(float x1, float y1, float x2, float y2, float r, float g, float b, float a, texture_slice *s) {
    push4(x1,y1,x2,y2,packColor(r, g, b, a),s);
}

// Someday, I am going to have to test to see if the texture coordinates here are correct.
// And on that day, I will be very sad.

void quadpile3::xysheet(cpVect a, cpVect b, cpFloat z1, cpFloat z2, unsigned int color, float *texes) {
	push(a.x, a.y, z1, color, texes+0);
	push(a.x, a.y, z2, color, texes+2);
	push(b.x, b.y, z2, color, texes+4);
	push(b.x, b.y, z1, color, texes+6);
}

void quadpile3::xzsheet(cpVect a, cpVect b, cpFloat y1, cpFloat y2, unsigned int color, float *texes) {
	push(a.x, y1, a.y, color, texes+0);
	push(a.x, y2, a.y, color, texes+2);
	push(b.x, y2, b.y, color, texes+4);
	push(b.x, y1, b.y, color, texes+6);
}

void quadpile3::yzsheet(cpVect a, cpVect b, cpFloat x1, cpFloat x2, unsigned int color, float *texes) {
	push(x1, a.x, a.y, color, texes+0);
	push(x2, a.x, a.y, color, texes+2);
	push(x2, b.x, b.y, color, texes+4);
	push(x1, b.x, b.y, color, texes+6);
}


void quadpile3::xyplane(cpVect a, cpVect b, cpFloat z, unsigned int color, float *texes) {
	push(a.x, a.y, z, color, texes+0);
	push(a.x, b.y, z, color, texes+2);
	push(b.x, b.y, z, color, texes+4);
	push(b.x, a.y, z, color, texes+6);
}

void quadpile3::xzplane(cpVect a, cpVect b, cpFloat y, unsigned int color, float *texes) {
	push(a.x, y, a.y, color, texes+0);
	push(a.x, y, b.y, color, texes+2);
	push(b.x, y, b.y, color, texes+4);
	push(b.x, y, a.y, color, texes+6);
}

void quadpile3::yzplane(cpVect a, cpVect b, cpFloat x, unsigned int color, float *texes) {
	push(x, a.x, a.y, color, texes+0);
	push(x, a.x, b.y, color, texes+2);
	push(x, b.x, b.y, color, texes+4);
	push(x, b.x, a.y, color, texes+6);
}

// Returns size of a single vertex in units sizeof(float)
int pile::stride() const {
	return 2 + useThree + useColor + useTexture*2;
}

state_basic *state_basic::single = new state_basic();
void state_basic::set(pile &q) {
	States(q.useTexture, q.useColor);
}

element_quad *element_quad::single = new element_quad();
void element_quad::draw(pile &q) {
	int vertsize = q.stride();
	int pushed = q.size()/vertsize/4; // This many quads
	megaIndexEnsure(pushed);

	glDrawElements(GL_TRIANGLES, pushed*6, GL_UNSIGNED_SHORT, &element_quad::megaIndex[0]); // *6 because enough indices for two triangles
}

void pile::draw() {
	stateSetter->set(*this);
	
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


void linepile::push(float x1, float y1, float x2, float y2, bool usecolor, float r, float g, float b, float a) {
    push(cpv(x1, y1), cpv(x2, y2), usecolor, r, g, b, a);
}

void linepile::push(cpVect v1, cpVect v2, bool usecolor, float r, float g, float b, float a) {
    cpVect vertices[2] =   { v1, v2 };
	unsigned int color;
    if (usecolor) color = packColor(r, g, b, a);
	for(int c = 0; c < 2; c++) {
		cpVect &v = vertices[c];
		push_back(v.x);
		push_back(v.y);
        if (usecolor)
            push_back(*((float *)&color));
	}		    
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