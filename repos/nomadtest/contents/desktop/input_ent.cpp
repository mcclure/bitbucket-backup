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
	
void inputdump_ent::insert(ent *_parent, int _prio) {
	ent::insert(_parent, _prio);
	
	InputRules::push(false);
	
	InputRuleSpec spec(kind, I_VACUUM);
	
	InputRules::rules()->load(spec);
}

void inputdump_ent::die() {
	InputRules::pop();
	
	ent::die();
}

void inputdump_ent::input(InputData *data) {
	const string &debugString = data->debugString();
	ERR("Input: %s\n", debugString.c_str());
}

void sticker::input(InputData *d) {
	if (d->inputcode == inputcode) {
		strength = d->strength;
		update(d);
	}
}

void switcher::input(InputData *d) {
	if (d->inputcode == inputcode) {
		down = d->axiscode & AXISCODE_RISE_MASK;
		update(d);
	}
}