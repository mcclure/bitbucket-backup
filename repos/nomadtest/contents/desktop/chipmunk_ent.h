/*
 *  chipmunk_ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/11/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _CHIPMUNK_ENT_H
#define _CHIPMUNK_ENT_H

#include "ent.h"
#include "chipmunk.h"

struct chipmunk_ent : public ent {
	cpSpace *space;
	cpBody *staticBody;

	chipmunk_ent() : ent(), space(NULL), staticBody(NULL) {}
	~chipmunk_ent();
	
	virtual void space_init();
    void insert(ent *_parent = NULL, int _prio = ENT_PRIO_DEFAULT);
    void tick();
};

struct cp_debug_ent : public ent {
	chipmunk_ent *parent;
	cp_debug_ent(chipmunk_ent *_parent) : ent(), parent(_parent) {}
	virtual void display(drawing *);
};

#endif