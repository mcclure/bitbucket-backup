#ifndef _PLAYTEST_H
#define _PLAYTEST_H

#include "program.h"

#if PLAYTEST_RECORD

/*
 *  playtest.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 3/25/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

struct recorder_auto : public automaton {
	recorder_auto();
	virtual ~recorder_auto();
	virtual void insert();
	virtual void die();
	virtual void handleEvent(Event *e);
	virtual void tick();
protected:
	FILE *recording;
};

#endif

#if PLAYTEST_RECORD || _DEBUG

struct chunk {
	uint32_t type;
	bool valid;
	vector<uint8_t> data;
	chunk(uint32_t _type);
	chunk(FILE *f);
	void load(FILE *f);
	void write(FILE *f);
	void clear();
};

struct playback_auto : public automaton {
	playback_auto(FILE *f, int _ticks_floor);
	virtual ~playback_auto();
	virtual void insert();
	virtual void tick();
	virtual void handleEvent(Event *e);
protected:
	int file_ticks_floor, file_ticks_at, real_ticks_floor, temp; // "temp" is in-file pointer
	FILE *play;
	chunk c; // current
	void prep();
};

struct playback_loader : public automaton {
	string filename;
	playback_loader(const string &_filename = string());
	string index();
	void load_from(int idx);
};

#endif

#endif /* _PLAYTEST_H */