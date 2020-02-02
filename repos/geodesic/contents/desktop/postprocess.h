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
	target_ent(texture_slice *initialInto, bool ownInto) : framebuffer(new framebuffer_source(initialInto, ownInto)), ownFramebuffer(true) {
		framebuffer->create();
	}
	target_ent() { if (ownFramebuffer) delete framebuffer; }
	void display(drawing *);
	void enter(); // Same question as above?
	void exit();
	virtual texture_slice *display_texture() {
		return framebuffer->outTexture;
	}
};

texture_slice *simple_texture(int w, int h);

// "_pwOrScale" is a scale if it's given by itself, a x pixel value if given with a _ph. Is that confusing?
struct filter : public ent {
	vector<target_ent *> target;
	vector<stateoid *> stateSetter;
	int pw, ph;
	ent *source; // used only to insert
	filter(ent *_source = NULL, stateoid *_stateSetter = NULL, double _pwOrScale = -1, double _ph = -1) : ent(), source(_source) {
		if (_stateSetter) addStateSetter(_stateSetter); // Are these worth it?
		if (_ph > 0) {
			size(_pwOrScale, _ph);
		} else {
			pw = surfacew; ph = surfaceh;
			if (_pwOrScale > 0)
				scale(_pwOrScale);
		}
	}
	void size(int _pw, int _ph) { pw = _pw; ph = _ph; }
	void scale(double by) { pw *= by; ph *= by; }
	void addStateSetter(stateoid *_stateSetter) { stateSetter.push_back(_stateSetter); }
	void inserting();
	
	virtual texture_slice *make_texture(int c, int of);
	virtual ent *make_splatter(texture_slice *from, stateoid *with, int c, int of);
};

struct fixed_filter : public filter {
	cpVect size, offset; // used only to insert
	fixed_filter(ent *_source = NULL, stateoid *_stateSetter = NULL, double _pw = -1, double _ph = -1, cpVect _size = cpv(1,1), cpVect _offset = cpvzero)
		: filter(_source, _stateSetter, _pw, _ph), size(_size), offset(_offset) {}
	ent *make_splatter(texture_slice *from, stateoid *with, int c, int of);
};

// A target ent which allows "feedback"
struct alternator : public target_ent {
	int lifetime; // Frames remaining or -1 for forever. 0 is "pause".
	splatter *splat;
	framebuffer_source *splat_framebuffer; // Typed copy of splat->src
	alternator(texture_slice *initialInto, bool ownInto, texture_slice *initialFrom, bool ownFrom, stateoid *stateSetter = NULL)
		: target_ent(initialInto, ownInto), lifetime(-1) {
		splat_framebuffer = new framebuffer_source(initialFrom, ownFrom);
		splat_framebuffer->create();
		
		display_ent *wrapper = new display_ent(); // Silly hack to make sure splatting terminates before any children are called.
		wrapper->insert(this);
		
		splat = new splatter(splat_framebuffer, true, stateSetter);
		splat->insert(wrapper);
	}
	~alternator() {
		delete splat;
	}
	void display(drawing *d) {
		if (lifetime != 0) { // Always render children, only swap when alive.
			target_ent::display(d);
			framebuffer->framebuffer_swap(splat_framebuffer);
		}
		if (lifetime > 0) {
			lifetime--;
		}
	}
	texture_slice *display_texture() {
		return splat_framebuffer->outTexture; // "InitialFrom"
	}
};

#endif