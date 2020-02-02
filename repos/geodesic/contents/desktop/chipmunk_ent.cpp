/*
 *  chipmunk_ent.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/11/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "chipmunk_ent.h"
#include "display_ent.h"
#include "program.h"

#define GRAVITY cpv(0.0f, 0.0f)

void chipmunk_ent::space_init() {
	space = cpSpaceNew();
	
	/* Next, you'll want to set the properties of the space such as the
     number of iterations to use in the constraint solver, the amount
     of gravity, or the amount of damping. In this case, we'll just
     set the gravity. */
	space->gravity = GRAVITY;
	
	/* This step is optional. While you don't have to resize the spatial
	 hashes, doing so can greatly increase the speed of the collision
	 detection. The first number should be the expected average size of
	 the objects you are going to have, the second number is related to
	 the number of objects you are putting. In general, if you have more
	 objects, you want the number to be bigger, but only to a
	 point. Finding good numbers to use here is largely going to be guess
	 and check. */
	cpSpaceResizeStaticHash(space, 0.1, 0.1);
	cpSpaceResizeActiveHash(space, 0.1, 200);
	
  /* This is the rigid body that we will be attaching our ground line
     segments to. We don't want it to move, so we give it an infinite
     mass and moment of inertia. We also aren't going to add it to our
     space. If we did, it would fall under the influence of gravity,
     and the ground would fall with it. */
	staticBody = cpBodyNew(INFINITY, INFINITY);
}

void chipmunk_ent::insert(ent *_parent, int _prio) {
	if (!space) space_init();
	
	ent::insert(_parent,_prio);
}

void chipmunk_ent::tick() {
	cpSpaceStep(space, 1.0/FPS);
}

chipmunk_ent::~chipmunk_ent() {
	cpSpaceFree(space);
}

void cp_debug_ent::display(drawing *d) {
//		cpSpaceHashEach(s->space->staticShapes, &display_object, s);
//		cpSpaceHashEach(s->space->activeShapes, &display_object, s);

	// Draw positions of bodies.
	cpArray *bodies = parent->space->bodies;
	
//	glBegin(GL_POINTS); {
		float py = 4.0/surfaceh;
		cpVect off = cpv(py, py);
		quadpile &into = d->get_quadpile(D_C, false, true);

		for(int i=0; i<bodies->num; i++){
			cpBody *body = (cpBody*)bodies->arr[i];
			cpVect from = cpvsub(body->p, off), to = cpvadd(body->p, off);
			cpVect verts[4] = RECT(from, to);
			into.push4(verts, 0xFF0000FF);
		}
//				glColor3f(1.0, 0.0, 0.0);
//				cpArrayEach(s->space->arbiters, &drawCollisions, NULL);
//	} glEnd();
		
#if 0
	glBegin(GL_LINES); { 
		for(int i=0; i<bodies->num; i++){
			cpBody *body = (cpBody*)bodies->arr[i];
			glColor3f(1.0, 0.0, 1.0);
			glVertex2f(body->p.x, body->p.y);
			glVertex2f(body->p.x+body->rot.x*36, body->p.y+body->rot.y*36);
		}
	} glEnd();
#endif
}