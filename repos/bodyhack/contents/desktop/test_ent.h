/*
 *  test_ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/9/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

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

#endif