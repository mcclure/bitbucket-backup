/*
 *  ent.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/26/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "ent.h"

vector<ent *> dying_ent;

#define ITERATE_CHILDREN(act) \
	for(int c = 0; c < ENT_PRIO_MAX; c++) \
		if (children[c]) \
			for(ent_iter i = children[c]->begin(); i != children[c]->end(); i++) \
				{ act; }

void ent::tick() {
    if (age() > 0) { // Is this necessary/correct?
        frame++;
    }
    if (rollover >= 0 && frame > rollover) {
        roll();
    }
	
	ITERATE_CHILDREN( (*i)->tick() );
}

void ent::next(int _state, int _rollover) {
	state = _state;
	rollover = _rollover;
	frame = 0;
}

void ent::roll() {
	next(state + 1);
}

void ent::display(drawing *d) {
	ITERATE_CHILDREN( (*i)->display(d) );
}

void ent::input(InputData *inp) {
	ITERATE_CHILDREN( (*i)->input(inp) );
}

void ent::die() {
    dying_ent.push_back(this);
}

void ent::insert(ent *_parent, int _prio) {	
	// Sanitize
	if (!_parent) _parent = global_ent;
	if (_prio < 0 || _prio >= ENT_PRIO_MAX) return;
	
	// Track what we've done
	parent = _parent; prio = _prio;
	
	inserting(); // Subclasses that don't care who their parent is implement this
	
	// Exactly which list will we be operating on?
	ent_list *&into = parent->children[prio];
	
	// We may need to create the list
	if (!into)
		into = new ent_list();
	
	// Add to list
	into->push_back(this);
	parent_anchor = --into->end();
};

ent::~ent() {
	// Make sure none of our children think we still exist
	ITERATE_CHILDREN( (*i)->parent = NULL );
	for(int c = 0; c < ENT_PRIO_MAX; c++) // Delete ent_lists
		delete children[c];

	if (parent) // Make sure our parent doesn't think we still exist
		parent->children[prio]->erase(parent_anchor);	
}

void ent::global_tick() {
	global_ent->tick();
	
	// Clean up anything that die()d
	if (!dying_ent.empty()) {
		for(int c = 0; c < dying_ent.size(); c++)
			delete dying_ent[c];
		dying_ent.clear();
	}
}