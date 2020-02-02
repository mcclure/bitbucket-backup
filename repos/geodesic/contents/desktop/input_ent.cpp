/*
 *  input_ent.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/16/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "input_ent.h"
#include "input.h"
#include "inputcodes.h"
	
input_vacuum_ent::input_vacuum_ent(InputKind _kind) : ent(), spec(_kind, I_VACUUM) {}
	
void input_vacuum_ent::insert(ent *_parent, int _prio) {
	ent::insert(_parent, _prio);
		
	InputRules::rules()->load(spec);
}

void input_vacuum_ent::die() {
	InputRules::rules()->unload(spec);
	
	ent::die();
}

void input_vacuum_ent::input(InputData *data) {
#ifdef SELF_EDIT
	const string &debugString = data->debugString();
	ERR("Input: %s\n", debugString.c_str());
#endif
}

void sticker::input(InputData *d) {
	if (d->inputcode == inputcode) {
		strength = d->strength;
		update(d);
	}
}

float sticker::stick(float gate) {
	return fabs(strength) > gate ? strength : 0;
}

void switcher::input(InputData *d) {
	if (d->inputcode == inputcode) {
		if (toggle)
			down = !down;
		else
			down = d->axiscode & AXISCODE_RISE_MASK;
		update(d);
	}
}