/*
 *  test_ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/9/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

// TODO: Somehow, this wound up also being home for a bunch of misc graphics routines. Split them out.

#ifndef _TEST_ENT_H
#define _TEST_ENT_H

#include "ent.h"

struct testdiamond : public ent {
	testdiamond() : ent() {}
	void display(drawing *);
};

struct testsquarefall : public ent {
	testsquarefall() : ent() {}
	void display(drawing *);
};

void testcross(slice *s);
void testfill(slice *s, unsigned int color);
void teststatic(slice *s, float low = 0.0, float high = 1.0);
void testcol(slice *s, int x, unsigned int color);
void testrow(slice *s, int y, unsigned int color);

void slice_copy_scaleup(slice *dst, slice *src, int scale);
void slice_copy_to_larger(slice *dst, slice *src, int dst_x = 0, int dst_y = 0, int dx = -1, int dy = -1);
void slice_copy_to_smaller(slice *dst, slice *src, int dst_x = 0, int dst_y = 0, int dx = -1, int dy = -1);

// (x%y) with C++11 sign rules.
// Assumes negative x and positive y
inline int neg_imod(int x, int y) {
	return y - 1 + (x % (-y));
}

// (x%y) with C++11 sign rules.
// Assumes positive y
inline int safe_imod(int x, int y) {
	return x > 0 ? x % y : neg_imod(x,y);
}

#endif