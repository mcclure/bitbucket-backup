/*
 *  input.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/16/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _INPUT_H
#define _INPUT_H

#include "kludge.h"
#include "program.h"
#include "ent.h"
#include "freeze.h"

#define INPUTCODE_GROUP_MASK 0xFF000000
#define INPUTCODE_ID_MASK    0x00FFFFFF
#define INPUTCODE_GROUP_OFFSET 24

#define INPUTCODE(group, id) ((id & INPUTCODE_ID_MASK) | ((group << INPUTCODE_GROUP_OFFSET) & INPUTCODE_GROUP_MASK))
#define INPUTCODE_GROUP(x) ((x & INPUTCODE_GROUP_MASK) >> INPUTCODE_GROUP_OFFSET)
#define INPUTCODE_ID(x) (x & INPUTCODE_ID_MASK)

// Identifiers used for both hats and axes.
// Oddities: "Xhat"/"Yhat" currently supported on axis…?
// Swap parity of RAW_MASK?
#define AXISCODE_RAW_MASK  0x80000000
// (SDLK_SCANCODE_MASK  is 0x40000000 so that's reserved)
#define AXISCODE_HIGH_MASK 0x20000000
#define AXISCODE_ZERO_MASK 0x10000000
#define AXISCODE_LOW_MASK  0x08000000
#define AXISCODE_XHAT_MASK 0x04000000
#define AXISCODE_YHAT_MASK 0x02000000
#define AXISCODE_ID_MASK   0x000000FF

#define AXISCODE_EVENT_MASK (AXISCODE_RAW_MASK | AXISCODE_HIGH_MASK | AXISCODE_ZERO_MASK | AXISCODE_LOW_MASK)
#define AXISCODE_DPAD_MASK  (AXISCODE_XHAT_MASK | AXISCODE_YHAT_MASK)
#define AXISCODE_META_MASK (AXISCODE_EVENT_MASK | AXISCODE_DPAD_MASK)

#define AXISCODE_RISE_MASK (AXISCODE_HIGH_MASK | AXISCODE_LOW_MASK)
#define AXISCODE_EDGE_MASK (AXISCODE_RISE_MASK | AXISCODE_ZERO_MASK)

#define AXISCODE_META_PASTE(meta, low) ((low & ~AXISCODE_META_MASK) | (meta & AXISCODE_META_MASK))

// SDL_NUM_SCANCODES is 512
#define AXISCODE_SDLK_MASK	(0x000001FF | SDLK_SCANCODE_MASK)

#define MASK_EQUAL(a,b,m)   (((a)&(m))==((b)&(m)))
#define MASK_OVERLAP(a,b,m) ((a)&(b)&(m))

#define AXISCODE_META(x) (x & AXISCODE_META_MASK)
#define AXISCODE_ID(x) (x & AXISCODE_ID_MASK)
#define AXISCODE_SDLK(x) (x & AXISCODE_SDLK_MASK)

#define AXISCODE_SET_ID(x, v)   x |= AXISCODE_ID(v)
#define AXISCODE_SET_SDLK(x, v) x |= AXISCODE_SDLK(v)

#define INPUTNAME_VACUUM "Vacuum"
#define INPUTNAME_RISE "Rise"
#define INPUTNAME_EDGE "Edge"
#define INPUTNAME_TOUCH "Touch"
#define INPUTNAME_KEY "Keyboard"
#define INPUTNAME_SYSTEM "System"

inline uint32_t AXISCODE(uint32_t _id, bool raw = true, bool low = false, bool zero=false, bool high = false, bool xhat = true, bool yhat = false) {
	return AXISCODE_ID(_id) | (raw?AXISCODE_RAW_MASK:0) | (low?AXISCODE_LOW_MASK:0) | (zero?AXISCODE_ZERO_MASK:0)
		| (high?AXISCODE_HIGH_MASK:0) | (xhat?AXISCODE_XHAT_MASK:0) | (yhat?AXISCODE_YHAT_MASK:0);
}

enum InputKind {
	InputKindInvalid, // Used only in storage
	InputKindVacuum,  // Used only in storage
	InputKindRise,    // Used only in storage
	InputKindEdge,    // Used only in storage
	InputKindTouch,
	InputKindKeyboard,
	InputKindAxis,
	InputKindHat,
	InputKindButton,
	InputKindSystem,   // Quit, hibernate, etc
};

enum {
	InputSystemInvalid,
	InputSystemQuit,
	InputSystemRecontroller,
};

struct InputRuleSpec {
	InputKind kind;
	uint32_t inputcode;
	uint32_t axiscode; // Possibly unused. Possibly contains a SDLK
	string devicename; // Possibly unused (for storage put in a dummy for human benefit)
	InputRuleSpec(InputKind _kind = InputKindInvalid, uint32_t _inputcode = 0, uint32_t _axiscode = 0, string _devicename = string()) : kind(_kind), inputcode(_inputcode), axiscode(_axiscode), devicename(_devicename) {} 
	string debugString();
	
	void f_load(FILE *f);
	void f_save(FILE *f);
};

struct ControllerRules {
	hash_map<uint32_t, uint32_t> axis; // Lookup by axis code
	hash_map<uint32_t, uint32_t> hat;  // Lookup by axis code
	hash_map<uint32_t, uint32_t> button;
	
	// Runtime state.
	hash_map<uint32_t, int32_t> axisState; // Lookup by axis id
	hash_map<uint32_t, int32_t> hatState;  // Lookup by hat id
	
	ControllerRules() {}
	ControllerRules(const ControllerRules &);
};

struct InputData;
struct ent;

struct InputRules : public freeze {
	uint32_t vacuum;	// All-events vacuum
	uint32_t rise;		// All-events vacuum but only counts "rising edges" (HIGH and LOW)
	uint32_t edge;		// All-events vacuum but only counts "edges" (transitions)
	uint32_t touch;		// Touch vacuum
	uint32_t keyboard;  // Whole keyboard vacuum
	hash_map<uint32_t, uint32_t> key; // Lookup by SDL_Keycode
	hash_map<string, ControllerRules *> controllerStorage; // Lookup by string-- for storage
	hash_map<uint32_t, ControllerRules *> controller; // Lookup by joystick id-- for use
	hash_map<uint32_t, uint32_t> system; // Lookup by InputSystem code
	
	InputRules() : freeze(), vacuum(0), rise(0), edge(0), touch(0), keyboard(0) {}
	InputRules(const InputRules &);
	~InputRules();
	vector<InputRuleSpec> dump();
	vector<InputRuleSpec> filter(vector<uint32_t> inputcodes);
	
	void load(const InputRuleSpec &rule);
	void unload(const InputRuleSpec &rule);
		
	void f_load(FILE *f);
	void f_save(FILE *f);
	
	ControllerRules *controllerFor(const string &name);
	ControllerRules *controllerFor(uint32_t _id);
	
	static InputRules *rules();
	
	void route(InputData *data, ent *target = NULL); // Gets "incomplete" inputdata, with no inputcode, axiscode, strength set, only raw members
	
protected:
	void routeHat(int32_t oldValue, uint32_t hatmask, ControllerRules *controller, InputData *data, ent *target);
	void routeHasAxiscode(ControllerRules *controller, InputData *data, ent *target);
	void routeAttempt(uint32_t axiscode, uint32_t inputcode, InputData *data, ent *target);
};

struct InputData {
	InputKind kind;
	uint32_t inputcode;
	
	// Derived
	uint32_t axiscode; // Combination of id and axiscode
	float strength; // -1 ... 1 of current value
	
	// Underlying data
	union {
		struct {
			touch_type touchkind;
			touch_rec touch;
		};
#ifdef TARGET_DESKTOP
		SDL_KeyboardEvent key;
		SDL_JoyAxisEvent jaxis;
		SDL_JoyHatEvent jhat;
		SDL_JoyButtonEvent jbutton;
#endif
		uint32_t systemkind;
	};

	uint32_t match(uint32_t axiscode);
	string debugString();
};

// Kludge
string nameForJoystick(int _id);

#endif