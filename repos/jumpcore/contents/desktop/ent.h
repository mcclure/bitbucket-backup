/*
 *  ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/26/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _ENT_H
#define _ENT_H

#include "kludge.h"
#include <list>

extern int ticks;
struct ent;
typedef list<ent *> ent_list;
typedef ent_list::iterator ent_iter;
extern ent *global_ent;

struct drawing;
struct InputRuleSpec;
struct InputData;

#define ENT_PRIO_DEFAULT 2
#define ENT_PRIO_MAX 5

// Autonomous entity
struct ent {
	ent_list *children[ENT_PRIO_MAX];
	
    int born;  // ticks born at
    int state; // which state are you at?
    int frame; // ticks within state
    int rollover; // what frame does state roll at?

	ent *parent; // Whose
	int prio;
    ent_iter parent_anchor;
    ent() : born(ticks), frame(0), state(0), rollover(-1), parent(NULL), prio(-1) {
		memset(children, 0, sizeof(children));
	}
    inline int age() { return ticks-born; }
    virtual void tick();
	virtual void roll();
	virtual void next(int _state, int _rollover=-1);
	virtual void display(drawing *);
	virtual void input(InputData *);
    virtual void die();
	
	virtual void inserting() {}
    virtual void insert(ent *_parent = NULL, int _prio = ENT_PRIO_DEFAULT);
    virtual ~ent();
	
	static void global_tick();
};

struct expiring_ent : public ent {
	int expired;
	expiring_ent() : ent(), expired(-1) {}
	void tick() {
		if (expired >= 0 && ticks > expired) {
			die();
		}
		ent::tick();
	}
	void expire() { expired = ticks; }
};

#endif