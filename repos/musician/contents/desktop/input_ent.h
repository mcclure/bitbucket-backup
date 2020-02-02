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

struct inputdump_ent : public ent {
	InputKind kind;
	
	inputdump_ent(InputKind _kind = InputKindVacuum) : ent(), kind(_kind) {}
	~inputdump_ent() {}
	
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
	bool down;
	switcher(uint32_t _inputcode) : ent(), inputcode(_inputcode) {}
	void input(InputData *);
	virtual void update(InputData *) {}
};

#endif
