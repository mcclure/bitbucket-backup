/*
 *  input_ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/16/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _INPUT_ENT_H
#define _INPUT_ENT_H

#include "kludge.h"
#include "ent.h"
#include "input.h"

struct input_vacuum_ent : public ent {
	InputRuleSpec spec;
	
	input_vacuum_ent(InputKind _kind = InputKindVacuum);
	
    void insert(ent *_parent = NULL, int _prio = ENT_PRIO_DEFAULT);
	void die();
	void input(InputData *);
};

struct sticker : public ent {
	uint32_t inputcode;
	float strength;
	sticker(uint32_t _inputcode) : ent(), inputcode(_inputcode), strength(0) {}
	void input(InputData *);
	virtual void update(InputData *) {} // InputData NOT guaranteed non-NULL
	float stick(float gate = 0.15);
};

struct switcher : public ent {
	uint32_t inputcode;
	bool down, toggle;
	switcher(uint32_t _inputcode, bool _toggle = false) : ent(), inputcode(_inputcode), down(false), toggle(_toggle) {}
	void input(InputData *);
	virtual void update(InputData *) {}
};

struct InputWishSpec {
	string value;
	uint32_t inputcode;
	uint32_t axiscode;
	InputWishSpec(const string &_value, uint32_t _inputcode, uint32_t _axiscode)
		: value(_value), inputcode(_inputcode), axiscode(_axiscode) {}
};

struct input_mapper : public ent {
	bool found_controllers;
	input_mapper();
	vector<InputRuleSpec> currentRules;
	vector<InputWishSpec> currentWishes;
	void clear();
	void assign();
	void addKeyboard(SDL_Scancode  value, uint32_t inputcode, uint32_t axiscode = AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	void addJoystick(const string &value, uint32_t inputcode, uint32_t axiscode = AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	void inserting();
	void input(InputData *);
};

#endif
