#ifndef _UTIL_PILE_H
#define _UTIL_PILE_H

/*
 *  util_pile.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 1/9/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */
 
#include "kludge.h"
#include "pile.h"

extern const int PHI;

void pile_cube(quadpile3 &q, glm::vec3 root = glm::vec3(0,0,0), glm::vec3 size = glm::vec3(1,1,1));
void pile_xzplane(pile &q, int xpoints, int zpoints=-1, element_free *triangle = NULL, element_free *wire = NULL);
void pile_tetrahedron(pile &q, element_free *triangle, element_free *wire);
void matrix_apply(const glm::mat4 &m, float *in, float *out);

// Assume useThree true; may crash otherwise. From/count are in points, not floats.
void pile_transform_apply(pile &q, const glm::mat4 &m, int from=0, int count=-1);
void pile_transform_push(pile &dst, pile &src, const glm::mat4 &m, int from=0, int count=-1);

void push_vertex(pile &dst, pile &src, int idx);
void pile_push_all(pile &dst, pile &src);

void pile_unpack(pile &dst, pile &src, element_free &e);

glm::vec3 glm_apply(const glm::vec3 &, const glm::mat4 &);

void pile_recolor(pile &q, int idx, unsigned int color);
void pile_rewrap(pile &q, int idx, float texcoord1, float texcoord2);


#endif