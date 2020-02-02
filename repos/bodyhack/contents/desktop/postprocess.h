/*
 *  postprocess.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/6/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _POSTPROCESS_H
#define _POSTPROCESS_H

#include "display_ent.h"

#define ENT_PRIO_FBTARGET (ENT_PRIO_DEFAULT-1)

struct framebuffer_source : public texture_source {
	GLuint framebuffer, renderbuffer;
	display_code code;
	framebuffer_source(texture_slice *s, bool own) : texture_source(s, own), framebuffer(0), renderbuffer(0) {}
	
	void vacate(); // Are these names too vague?
	void create();
	void enter(); // Take drawing as arguments, for restore?
	void exit();
	void framebuffer_swap(framebuffer_source *);
	cpVect size();
	~framebuffer_source() { vacate(); }
};

// Wrapper where a set of children is drawn "inside" a framebuffer or whatever
// TODO: Should just be a special case of viewport ents?
struct target_ent : public display_ent {
	framebuffer_source *framebuffer;
	bool ownFramebuffer;
	target_ent(framebuffer_source *_framebuffer, bool _ownFramebuffer = false) : framebuffer(_framebuffer), ownFramebuffer(_ownFramebuffer) {}
	target_ent() { if (ownFramebuffer) delete framebuffer; }
	void display(drawing *);
	void enter(); // Same question as above?
	void exit();
};

struct alternator : public target_ent {
	splatter *splat;
	framebuffer_source *splat_framebuffer; // Typed copy of splat->src
	alternator(texture_slice *initialInto, bool ownInto, texture_slice *initialFrom, bool ownFrom, stateoid *stateSetter)
		: target_ent(new framebuffer_source(initialInto, ownInto), true) {
		framebuffer->create();
		
		splat_framebuffer = new framebuffer_source(initialFrom, ownFrom);
		splat_framebuffer->create();
		
		splat = new splatter(splat_framebuffer, true, stateSetter);
		splat->insert(this);
	}
	~alternator() {
		delete splat;
	}
	void display(drawing *d) {
		target_ent::display(d);
		framebuffer->framebuffer_swap(splat_framebuffer);
	}
	texture_slice *display_texture() {
		return splat_framebuffer->outTexture; // "InitialFrom"
	}
};


#endif