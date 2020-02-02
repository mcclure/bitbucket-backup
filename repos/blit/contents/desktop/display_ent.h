/*
 *  display_ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/28/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _DISPLAY_ENT_H
#define _DISPLAY_ENT_H

#include "kludge.h"
#include "display.h"
#include "ent.h"

#define RECT(from, to) { cpv((from).x,(from).y), cpv((from).x, (to).y), cpv( (to).x, (to).y), cpv( (to).x,(from).y), }

// Infrastructure

struct texture_source {
	texture_slice *outTexture;
	bool ownTexture;
	display_code code;
	texture_source(texture_slice *s, bool own = false);
	texture_source(const char *name);
	virtual ~texture_source();
	void texture_load(texture_slice *s, bool own = false);
	void texture_load(const char *name);
};

// Ents

struct lockout : public ent {
	lockout() : ent() {}
	void display(drawing *);
};

struct display_ent : public ent {
	display_ent() : ent() {}
	void display(drawing *);	
};

// Things that draw other things

struct eraser : public ent {
	float r, g, b, a;
	eraser(float _r = 0, float _g = 0, float _b = 0, float _a = 1) : r(_r), g(_g), b(_b), a(_a) {}
	void display(drawing *);
};

struct splatter : public ent {
	texture_source *src;
	bool ownSrc;
	stateoid *stateSetter; // Always owned
	splatter(texture_source *_src, bool _ownSrc = false, stateoid *_stateSetter = NULL) : ent(), src(_src), ownSrc(_ownSrc) {
		stateSetter = _stateSetter ? _stateSetter : state_basic::single;
	}
	~splatter() { if (ownSrc) delete src; delete stateSetter; }
	void display(drawing *);
};

struct fixed_splatter : public splatter {
	cpVect size, offset;
	fixed_splatter(texture_source *_src, bool _ownSrc = false, stateoid *_stateSetter = NULL, cpVect _size = cpv(1,1), cpVect _offset = cpvzero) : splatter(_src, _ownSrc, _stateSetter), size(_size), offset(_offset) {}
	void display(drawing *);
};

struct boxer : public ent {
	display_code code;
	cpVect size, offset;
	boxer(cpVect _size = cpv(1,1), cpVect _offset = cpvzero, display_code _code = D_P) : ent(), size(_size), offset(_offset), code(_code) {}
	void display(drawing *);
};

// "kvm" utilities

template<class T>
struct kvm {
	vector<T> targets;
	int focus;
	kvm() : focus(0) {}
	virtual void pick(int _focus) { focus = _focus % targets.size(); }
	virtual void nextTarget(int by = 1) { pick(focus + by); }
	virtual void addTarget(T target) { targets.push_back(target); }
};

// Has many children, but only shows one at a time
struct display_kvm : public ent, public kvm<ent *> {
	display_kvm() : ent(), kvm<ent *>() {}
	void addTarget(ent *target) {
		kvm<ent *>::addTarget(target);
		target->insert(this);
	}
	void display(drawing *d) { // DON'T call super implementation
		if (targets.size()) {
			targets[focus]->display(d);
		}
	}
};

// Texture source for several textures, but only shows one at a time
struct source_kvm : public texture_source, public kvm<texture_slice *> {
	source_kvm() : texture_source(NULL, false), kvm<texture_slice *>() {}
	void pick(int _focus) {
		kvm<texture_slice *>::pick(_focus);
		outTexture = targets[focus];
	}
	void addTarget(texture_slice *target) {
		if (!targets.size())
			outTexture = target;
		kvm<texture_slice *>::addTarget(target);
	}
};

struct display_gate : public ent {
	bool visible;
	display_gate() : ent(), visible(true) {}
	void display(drawing *d) { // MAYBE call super implementation
		if (visible) {
			ent::display(d);
		}
	}
};

// Misc

// Saves a series of screenshots, when a particular inputcode is received.
struct screenshotter : public ent {
	int snapshots;
	string filename;
	uint32_t trigger;
	texture_source *texture;
	
	screenshotter(uint32_t _trigger, texture_source *_texture = NULL, string basename="screenshot");
	void input(InputData *data);
	void quickTake(texture_slice *from = NULL);
	
	// Take one screenshot. Right now. Given filename. Given texture. No questions asked.
	static void take(string filename, GLuint texture);
};

#endif