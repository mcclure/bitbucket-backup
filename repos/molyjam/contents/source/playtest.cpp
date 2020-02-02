/*
 *  playtest.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 3/25/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "playtest.h"
#include <sstream>

#define HYPERVERBAL 0
#define PLAYBACK_FPS 60.0 /* TODO should be global for project. */

#if HYPERVERBAL
#define HYPERERR ERR
#else
#define HYPERERR EAT
#endif

#if PLAYTEST_RECORD || _DEBUG

#include "terminal.h"

#define BFWRITE(x) (sizeof(x) == fwrite((char *) &(x), 1, sizeof(x), f))
#define BFREAD(x) (sizeof(x) == fread((char *) &(x), 1, sizeof(x), f))
#define BWRITE(x) temp = c.data.size(); c.data.resize(temp + sizeof(x)); memcpy(&c.data[temp], &x, sizeof(x)); HYPERERR("Write %d\n", *((int *)&x));
#define BREAD(x) memcpy(&(x), &c.data[temp], sizeof(x)); temp += sizeof(x); HYPERERR("Read %d\n", *((int *)&x));
#define DEFAULTDAT (PROGDIR "recording.dat")

void set_time(uint32_t now) {
	srandom(now);
	terminal_auto::singleton()->inject_global("use_seed", (int)now);
}

enum {
	chunk_unknown = 0,
	chunk_header,
	chunk_inputevent,
};
chunk::chunk(uint32_t _type) : type(_type), valid(true) {}

chunk::chunk(FILE *f) : valid(true) {
	load(f);
}

void chunk::load(FILE *f) {
	uint32_t size;
	valid = valid && BFREAD(type);
	valid = valid && BFREAD(size);
	if (valid) {
		data.resize(size);
		valid = size == fread(&data[0], 1, size, f);
	}
}

void chunk::write(FILE *f) {
	uint32_t size = data.size();
	BFWRITE(type);
	BFWRITE(size);
	if (size) {
		fwrite(&data[0], 1, size, f);
	}
}

void chunk::clear() {
	type = 0;
	valid = true;
	data.clear();
}

#endif

#if PLAYTEST_RECORD

// Automated record/playback
// Currently keyboard only

recorder_auto::recorder_auto() : recording(NULL) {}
recorder_auto::~recorder_auto() { if (recording) fclose(recording); }
void recorder_auto::insert() {
	recording = fopen(DEFAULTDAT, "ab");
	{
		uint32_t temp;
		uint32_t now = time(NULL);
		chunk c(chunk_header);
		BWRITE(ticks);
		BWRITE(now);
		c.write(recording);
		
		set_time(now);
	}
	
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN);
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYUP);
}
void recorder_auto::die() {
}
void recorder_auto::handleEvent(Event *_e) {
	if (done) return;
	
	if(_e->getDispatcher() == cor->getInput()) {
		chunk c(chunk_inputevent);
		size_t temp;
		int eventCode = _e->getEventCode();
		HYPERERR("\nNEW CHUNK\n");
		
		InputEvent *e = (InputEvent*)_e;
		BWRITE(ticks);
		BWRITE(eventCode);
		BWRITE(e->mouseButton);
		BWRITE(e->mousePosition);
		BWRITE(e->key);
		BWRITE(e->charCode);
		BWRITE(e->timestamp);
		BWRITE(e->joystickDeviceID);
		BWRITE(e->joystickAxisValue);
		BWRITE(e->joystickButton);
		BWRITE(e->joystickAxis);
		c.write(recording);
	}
}
void recorder_auto::tick() {
	fflush(recording);
}

#endif

#if PLAYTEST_RECORD || _DEBUG

playback_auto::playback_auto(FILE *f, int _ticks_floor)
	: play(f), c(chunk_unknown), file_ticks_floor(_ticks_floor), file_ticks_at(0), real_ticks_floor(0), temp(0) {
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN);
}
playback_auto::~playback_auto() {
	fclose(play);
}
void playback_auto::prep() {
	c.clear();
	c.load(play);
	HYPERERR("\nNEW CHUNK\n");
	if (!c.valid || c.type == chunk_header) {
		ERR("\n ---- PLAYBACK DONE --- \n");
		die();
	} else {
		temp = 0;
		BREAD(file_ticks_at);
	}
}
void playback_auto::insert() {
	automaton::insert();
	real_ticks_floor = ticks;
	prep();
	
#if VISIBLE_TERMINAL
	terminal_auto::singleton()->set_hidden(true);
#endif
}
void playback_auto::tick() {
	while (!done) {
		int file_tick = file_ticks_at - file_ticks_floor;
		int real_tick = ticks - real_ticks_floor;
		if (real_tick >= file_tick) {
			HYPERERR("CONSUME at tick %d\n", file_tick);
			
			InputEvent *e = new InputEvent();
			int eventCode;
			BREAD(eventCode);
			BREAD(e->mouseButton);
			BREAD(e->mousePosition);
			BREAD(e->key);
			BREAD(e->charCode);
			BREAD(e->timestamp);
			BREAD(e->joystickDeviceID);
			BREAD(e->joystickAxisValue);
			BREAD(e->joystickButton);
			BREAD(e->joystickAxis);	
			cor->getInput()->dispatchEvent(e, eventCode);
			
			prep();
		} else {
			break;
		}
	}
}

// Silly emergency controls
// F11 to skip forward 10sec in playthrough
// F12 to see how long until next user keypress
void playback_auto::handleEvent(Event *e) {
	if (done) return;
	
	if(e->getDispatcher() == cor->getInput()) {
		InputEvent *inputEvent = (InputEvent*)e;
		
		switch(e->getEventCode()) {
			case InputEvent::EVENT_KEYDOWN: {
				int code = inputEvent->keyCode();
				switch (code) {
					case KEY_F11:
						ticks += PLAYBACK_FPS*10;
					case KEY_F12: {
						int file_tick = file_ticks_at - file_ticks_floor;
						int real_tick = ticks - real_ticks_floor;
						ERR("Next event in %lf secs\n", (file_tick - real_tick)/PLAYBACK_FPS);
					} break;
				}
			} break;
		}
	}
}

playback_loader::playback_loader(const string &_filename) {
	if (_filename.empty())
		filename = DEFAULTDAT;
	else
		filename = _filename;
}

string playback_loader::index() {
	FILE *f = fopen(DEFAULTDAT, "rb");
	ostringstream o;
	unsigned idx = 0, packetcount = 0, ticks_start = 0, ticks_most = 0;
	
	if (!f) return string();
	
	while (1) {
		chunk c(f);
		int temp = 0;
		
		if (!c.valid || c.type == chunk_header) {
			if (idx != 0) {
				o << idx << ": " << packetcount << ", " << (ticks_most - ticks_start)/PLAYBACK_FPS << " secs" << std::endl;
				packetcount = 0; ticks_start = 0; ticks_most = 0;
			}
			if (c.type == chunk_header) {
				BREAD(ticks_start);
				idx++;
			}
		} else {
			unsigned int newticks;
			BREAD(newticks); ticks_most = max(newticks, ticks_most);
			packetcount++;
		}
		
		if (!c.valid)
			break;
	}
	fclose(f);
	return o.str();
}

void playback_loader::load_from(int idx) {
	FILE *f = fopen(DEFAULTDAT, "rb");
	unsigned at_idx = 0;
	
	if (!f) return;
	
	while (1) {
		chunk c(f);
		int temp = 0;

		if (!c.valid) {
			ERR("FAILED TO LOAD PLAYBACK\n");
			fclose(f);
			break;
		}
		
		if (c.type == chunk_header) {
			at_idx++;
			if (idx == at_idx) { 
				int ticks_floor; uint32_t now;
				BREAD(ticks_floor);
				BREAD(now);
				set_time(now);
				(new playback_auto(f, ticks_floor))->insert();
				break;
			}
		}
	}
}

#endif