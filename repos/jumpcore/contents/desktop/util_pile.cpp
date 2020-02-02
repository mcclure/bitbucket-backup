/*
 *  util_pile.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 1/9/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "game.h"
#include "display_ent.h"
#include "test_ent.h"
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "display_ent.h"
#include "glCommonMatrix.h"
#include "util_pile.h"

void pile_cube(quadpile3 &q, glm::vec3 root, glm::vec3 size) {
	#define CF(power, index) (root[index] + power*size[index])
	cpVect corners[4] = {cpv(CF(0, 0),CF(0, 1)), cpv(CF(0, 0),CF(1, 1)), cpv(CF(1, 0),CF(1, 1)), cpv(CF(1, 0),CF(0, 1))};
	
	for(int c = 0; c < 4; c++) {
		q.xysheet(corners[c], corners[(c+1)%4], CF(0, 2), CF(1, 2));
	}
	q.xyplane(corners[0], corners[2], CF(0, 2));
	q.xyplane(corners[0], corners[2], CF(1, 2));
}

void matrix_apply(const glm::mat4 &m, float *in, float *out) {
       glm::vec4 v = glm::vec4(in[0], in[1], in[2], 1);
       v = m * v;
       out[0] = v[0]/v[3]; out[1] = v[1]/v[3]; out[2] = v[2]/v[3];
}

void pile_xzplane(pile &q, int xpoints, int zpoints, element_free *triangle, element_free *wire) {
	if (zpoints < 0) zpoints = xpoints;
	
	for (int x=0; x<xpoints; x++ ) { // "row/height"
		for (int z=0; z<zpoints; z++ ) { // "col/width"
			q.push(x/float(xpoints), 0, z/float(zpoints));

			#define F_I(x, z) ((z) + ((x)*zpoints))
			int root = F_I(x,z), nz = F_I(x,z+1), nx = F_I(x+1, z), nn = F_I(x+1, z+1);
			bool xvis = x < xpoints-1, zvis = z < zpoints-1;
			
			if (triangle && xvis && zvis) {
				triangle->push(root); triangle->push(nx); triangle->push(nz);
				triangle->push(nz); triangle->push(nx); triangle->push(nn);
			}
			
			if (wire) {
				if (xvis) { wire->push(root); wire->push(nx); }
				if (zvis) { wire->push(nz); wire->push(root); }
				if (xvis && zvis) { wire->push(nx); wire->push(nz); }
			}
		}
	}
}

// Notice the weird mix of models here-- for some kinds of pile manipulation operations you have a method on pile(),
// for some kinds you have a standalone function in this file. For some kinds of geometry generation you have a pile
// subclass with specialized push_ functions, for others you have special functions in this file.
// This is all extremely low hanging fruit to refactor.

const int PHI = (1.0 + sqrt(5.0)) / 2.0;

// Based on http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
void pile_tetrahedron(pile &q, element_free *triangle, element_free *wire) {
	// create 12 vertices of a icosahedron
	float t = PHI;

	q.push(-1,  t,  0);
	q.push( 1,  t,  0);
	q.push(-1, -t,  0);
	q.push( 1, -t,  0);
	
	q.push( 0, -1,  t);
	q.push( 0,  1,  t);
	q.push( 0, -1, -t);
	q.push( 0,  1, -t);
	 
	q.push( t,  0, -1);
	q.push( t,  0,  1);
	q.push(-t,  0, -1);
	q.push(-t,  0,  1);

	#define TETRAHEDRON_FACES (5*4)
	const float faces[TETRAHEDRON_FACES][3] = {
		// 5 faces around point 0
		{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},

		// 5 adjacent faces
		{1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
		 
		// 5 faces around point 3
		{3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},

		// 5 adjacent faces
		{4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1},
	};

	if (triangle || wire) {
		for(int c = 0; c < TETRAHEDRON_FACES; c++) {
			const float *face = &faces[c][0];
			for(int d = 0; d < 3; d++) {
				if (triangle) triangle->push(face[d]);
				if (wire) {
					wire->push(face[d]);
					wire->push(face[(d+1)%3]);
				}
			}
		}
	}
}

void pile_transform_apply(pile &q, const glm::mat4 &m, int from, int count) {
	int stride = q.stride();
	if(count<0) count = q.vertices()-from;
	for(int c = 0; c < count; c++) {
		float *target = &q[(c+from)*stride];
		matrix_apply(m, target, target);
	}
}

// This isn't quite right
void pile_transform_push(pile &dst, pile &src, const glm::mat4 &m, int from, int count) {
	int srcStride = src.stride();
	int initialDst = dst.vertices();
	int absoluteFrom = from*srcStride;
	for(int c = 0; c < count*srcStride; c++) {
		dst.push_back( src[absoluteFrom+c] );
	}
	pile_transform_apply(dst, m, initialDst, count);
}

glm::vec3 glm_apply(const glm::vec3 &_v, const glm::mat4 &m) {
	glm::vec4 v = glm::vec4(_v, 1.0)*m;
	return glm::vec3(v.x,v.y,v.z);
}

static unsigned int whiteVariable = WHITE;

void push_vertex(pile &dst, pile &src, int idx) {
	int base = idx * src.stride();
	dst.push_back(src[base++]); // x
	dst.push_back(src[base++]); // y
	float value;
	
	value = src.useThree ? src[base++] : 0; // z
	if (dst.useThree) dst.push_back(value);
	
	value = src.useColor ? src[base++] : *((float *)&whiteVariable); // color
	if (dst.useColor) dst.push_back(value);
	
	if (dst.useTexture) { // texts -- last field
		if (src.useTexture) {
			dst.push_back(src[base++]);
			dst.push_back(src[base++]);
		} else {
			dst.push_back(0);
			dst.push_back(0);
		}
	}
}

void pile_push_all(pile &dst, pile &src) {
	int count = src.vertices();
	for(int c = 0; c < count; c++)
		push_vertex(dst, src, c);
}

void pile_unpack(pile &dst, pile &src, element_free &e) {
	int count = e.index.size();
	for(int c = 0; c < count; c++)
		push_vertex(dst, src, e.index[c]);
}

void pile_recolor(pile &q, int idx, unsigned int color) {
	if (q.useColor) {
		int offset = 2 + q.useThree; // Will break if I ever add custom attributes!
		int stride = q.stride();
		*((unsigned int *)&q[stride*idx + offset]) = color;
	}
}

void pile_rewrap(pile &q, int idx, float texcoord1, float texcoord2) {
	if (q.useTexture) {
		int offset = 2 + q.useThree + q.useColor;
		int stride = q.stride();
		q[stride*idx + offset]     = texcoord1;
		q[stride*idx + offset + 1] = texcoord2;
	}
}
