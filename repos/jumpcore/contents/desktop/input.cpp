/*
 *  input.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/16/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "input.h"
#include "inputcodes.h"
#include <sstream>

// FIXME: Should use different thresholds for rising and falling?
#define AXIS_THRESHOLD 16380

// TODO: Before there was a push/pop mechanism, it should come back
static vector<InputRules> rulestack;

typedef hash_map<string, ControllerRules *>::iterator controller_iter;
typedef hash_map<uint32_t, uint32_t>::iterator intmap_iter;

static vector<string> jnames;

string nameForJoystick(int _id) {
	const char *name = SDL_GameControllerNameForIndex(_id);
	if (name)
		return name;
	name =  SDL_JoystickNameForIndex(_id);
	if (name)
		return name;
	return "[INVALID]";
}

string InputKindString(InputKind k) {
	switch(k) {
		case InputKindInvalid: return "Invalid";
		case InputKindVacuum: return "Vacuum";
		case InputKindRise: return "Rise";
		case InputKindEdge: return "Edge";
		case InputKindTouch: return "Touch";
		case InputKindKeyboard: return "Keyboard";
		case InputKindAxis: return "Axis";
		case InputKindHat: return "Hat";
		case InputKindButton: return "Button";
		case InputKindSystem: return "System";
		default: return "Corrupt";
	}
}

string AxisCodeString(uint32_t a) {
	std::stringstream buffer;
	buffer << hex << a << " (";
	if (a&AXISCODE_RAW_MASK) buffer << "*";
	if (a&AXISCODE_HIGH_MASK) buffer << "H";
	if (a&AXISCODE_ZERO_MASK) buffer << "Z";
	if (a&AXISCODE_LOW_MASK)  buffer << "L";
	if (a&AXISCODE_XHAT_MASK) buffer << "X";
	if (a&AXISCODE_YHAT_MASK) buffer << "Y";
	buffer << " " << dec << AXISCODE_SDLK(a) << ")"; // Assume sdlk because bigger. Valid... for now.
	return buffer.str();
}

ControllerRules::ControllerRules(const ControllerRules &other) : axis(other.axis), hat(other.hat), button(other.button), axisState(other.axisState), hatState(other.hatState) {
}

InputRules::InputRules(const InputRules &other) : vacuum(other.vacuum), rise(other.rise), edge(other.edge), touch(other.touch), keyboard(other.keyboard), key(other.key) {
	for(controller_iter i = controllerStorage.begin(); i != controllerStorage.end(); i++)
		controllerStorage[i->first] = new ControllerRules(*(i->second));
}

InputRules::~InputRules() {
	for(controller_iter i = controllerStorage.begin(); i != controllerStorage.end(); i++)
		delete i->second;
}

vector<InputRuleSpec> InputRules::dump() {
	vector<InputRuleSpec> result;
	if (vacuum) result.push_back(InputRuleSpec(InputKindVacuum, vacuum, AXISCODE_HIGH_MASK, INPUTNAME_VACUUM)); // name and mask redundant
	if (rise)   result.push_back(InputRuleSpec(InputKindRise, rise, AXISCODE_RISE_MASK, INPUTNAME_RISE)); // name and mask redundant
	if (edge)   result.push_back(InputRuleSpec(InputKindEdge, edge, AXISCODE_EDGE_MASK, INPUTNAME_EDGE)); // name and mask redundant
	if (touch) result.push_back(InputRuleSpec(InputKindTouch, touch, 0, INPUTNAME_TOUCH));
	if (keyboard) result.push_back(InputRuleSpec(InputKindKeyboard, keyboard, 0, INPUTNAME_KEY)); // 0 is vacuum
	for(intmap_iter i = key.begin(); i != key.end(); i++) {
		result.push_back(InputRuleSpec(InputKindKeyboard, i->second, i->first, INPUTNAME_KEY));
	}
	for(controller_iter i = controllerStorage.begin(); i != controllerStorage.end(); i++) {
		for(intmap_iter k = i->second->axis.begin(); k != i->second->axis.end(); k++)
			result.push_back(InputRuleSpec(InputKindAxis, k->second, k->first, i->first));
		for(intmap_iter k = i->second->hat.begin(); k != i->second->hat.end(); k++)
			result.push_back(InputRuleSpec(InputKindHat, k->second, k->first, i->first));
		for(intmap_iter k = i->second->button.begin(); k != i->second->button.end(); k++)
			result.push_back(InputRuleSpec(InputKindButton, k->second, k->first, i->first));
	}
	for(intmap_iter i = system.begin(); i != system.end(); i++) {
		result.push_back(InputRuleSpec(InputKindSystem, i->second, i->first, INPUTNAME_SYSTEM));
	}
	return result;
}

// Returns InputKindInvalid if it doesn't find something. OK?
vector<InputRuleSpec> InputRules::filter(vector<uint32_t> inputcodes) {
	hash_map<uint32_t, InputRuleSpec> apply;
	vector<InputRuleSpec> source = dump();
	vector<InputRuleSpec> result;
	for(vector<InputRuleSpec>::iterator i = source.begin(); i != source.end(); i++)
		apply[i->inputcode] = *i;
	for(vector<uint32_t>::iterator i = inputcodes.begin(); i != inputcodes.end(); i++) {
		if (apply.count(*i))
			result.push_back(apply[*i]);
		else
			result.push_back(InputRuleSpec(InputKindInvalid));
	}
	return result;
}

ControllerRules *InputRules::controllerFor(const string &name) {
	if (controllerStorage.count(name)) {
		return controllerStorage[name];
	} else {
		ControllerRules *c = new ControllerRules();
		controllerStorage[name] = c;
		return c;
	}
}

ControllerRules *InputRules::controllerFor(uint32_t _id) {
	if (controller.count(_id)) {
		return controller[_id];
	} else {
		ControllerRules *c = controllerFor( nameForJoystick(_id) );
		controller[_id] = c;
		return c;
	}
}

InputRules *InputRules::rules() {
	if (rulestack.empty())
		rulestack.push_back(InputRules());
	return &rulestack.back();
}

#define DPAD_THRESHOLD 16380

int axisdir(int32_t v) {
	if (v > DPAD_THRESHOLD) return 1;
	else if (v < -DPAD_THRESHOLD) return -1;
	else return 0;
}

void InputRules::route(InputData *data, ent *target) {
	if (!target) target = global_ent;
	ControllerRules *controller = NULL;
	int32_t oldValue;
	
	// Set up axiscode. Notice: All except Axis defer strength; Hat also defers some of axiscode.
	data->axiscode = AXISCODE_RAW_MASK;
	data->strength = 0;
	
	switch (data->kind) {
		case InputKindTouch: {
			if (data->touchkind == touch_down) data->axiscode |= AXISCODE_RISE_MASK;
			else if (data->touchkind == touch_up || data->touchkind == touch_cancel) data->axiscode |= AXISCODE_ZERO_MASK;
			// TODO: strength should be based on mask, not edge.
		} break;
		case InputKindKeyboard: {
			AXISCODE_SET_SDLK(data->axiscode, data->key.keysym.sym);
			if (data->key.type == SDL_KEYDOWN) data->axiscode |= AXISCODE_RISE_MASK;
			else data->axiscode |= AXISCODE_ZERO_MASK;
		} break;
		case InputKindButton: {
			controller = controllerFor(data->jbutton.which);
			AXISCODE_SET_ID(data->axiscode, data->jbutton.button);
			
			if (data->jbutton.type == SDL_JOYBUTTONDOWN) data->axiscode |= AXISCODE_RISE_MASK;
			else data->axiscode |= AXISCODE_ZERO_MASK;
		} break;
		case InputKindHat: {
			controller = controllerFor(data->jhat.which);
			AXISCODE_SET_ID(data->axiscode, data->jhat.hat);
			
			int32_t &state = controller->axisState[data->jhat.hat];
			oldValue = state;
			state = data->jhat.value;
		} break;
		case InputKindAxis: {
			controller = controllerFor(data->jaxis.which);
			AXISCODE_SET_ID(data->axiscode, data->jaxis.axis);
			
			int32_t &state = controller->axisState[data->jaxis.axis];
			oldValue = state;
			state = data->jaxis.value;
			int newd = axisdir(state), oldd = axisdir(oldValue);
			if (newd != oldd) {
				if (newd == 1)
					data->axiscode |= AXISCODE_HIGH_MASK;
				else if (newd == -1)
					data->axiscode |= AXISCODE_LOW_MASK;				
				else
					data->axiscode |= AXISCODE_ZERO_MASK;
			}
		} break;
		case InputKindSystem: {
			AXISCODE_SET_ID(data->axiscode, data->systemkind);
		} break;
	}
	
	if (data->kind == InputKindHat) {
		routeHat(oldValue, AXISCODE_XHAT_MASK, controller, data, target);
		routeHat(oldValue, AXISCODE_YHAT_MASK, controller, data, target);
	} else {
		routeHasAxiscode(controller, data, target);
	}
}
		
// Notice ControllerRules gets passed forward indefinitely
void InputRules::routeHat(int32_t oldValue, uint32_t hatmask, ControllerRules *controller, InputData *data, ent *target) {
}

// Wondering why an event handler isn't being called? Then set a breakpoint here
void InputRules::routeHasAxiscode(ControllerRules *controller, InputData *data, ent *target) {
	// Now, and only now that we have a known axiscode, set strength
	if (data->kind == InputKindAxis) {
		data->strength = float(data->jaxis.value)/SHRT_MAX;
	} else {
		if (data->axiscode & AXISCODE_RISE_MASK) data->strength = 1;
	}

	// Route via each possible path
	if (vacuum)
		routeAttempt(data->axiscode, vacuum, data, target); // NOTICE THE WEIRD HACK TO GUARANTEE AXISCODE ACCEPTANCE
	if (rise)
		routeAttempt(AXISCODE_META_PASTE(AXISCODE_RISE_MASK, data->axiscode), rise, data, target);
	if (edge)
		routeAttempt(AXISCODE_META_PASTE(AXISCODE_EDGE_MASK, data->axiscode), edge, data, target);

	switch (data->kind) {
		case InputKindTouch: {
			if (touch)
				routeAttempt(data->axiscode, touch, data, target);
		} break;
		case InputKindKeyboard: {
			if (keyboard)
				routeAttempt(AXISCODE_META(data->axiscode), keyboard, data, target); // Similar weird hack, but we want SDLK == 0
				
			for(intmap_iter i = key.begin(); i != key.end(); i++)
				routeAttempt(i->first, i->second, data, target);
		} break;
		case InputKindButton: {
			for(intmap_iter i = controller->button.begin(); i != controller->button.end(); i++)
				routeAttempt(i->first, i->second, data, target);
		} break;
		case InputKindAxis: { 
			for(intmap_iter i = controller->axis.begin(); i != controller->axis.end(); i++)
				routeAttempt(i->first, i->second, data, target);
		} break;
		case InputKindHat: {
			for(intmap_iter i = controller->hat.begin(); i != controller->hat.end(); i++)
				routeAttempt(i->first, i->second, data, target);
		} break;
		case InputKindSystem: {
			for(intmap_iter i = system.begin(); i != system.end(); i++)
				routeAttempt(i->first, i->second, data, target);
		} break;
	}
}

// Axiscode is "axiscode to match"
void InputRules::routeAttempt(uint32_t axiscode, uint32_t inputcode, InputData *data, ent *target) {
	uint32_t match = data->match(axiscode);
	if (!match) return;
	
	// This feels like I wrote myself into a corner. We have to modify the structure, then revert it...
	uint32_t original_axiscode = data->axiscode;
	float original_strength = data->strength;
	
	data->axiscode = AXISCODE_META_PASTE(match, data->axiscode);
		
	if ( (axiscode & AXISCODE_LOW_MASK) && !(axiscode & AXISCODE_HIGH_MASK) ) // If original spec is LOW only, assume user wants inverted
		data->strength *= -1;
		
	data->inputcode = inputcode;
	
	target->input(data);
	
	data->axiscode = original_axiscode;
	data->strength = original_strength;
}

string InputRuleSpec::debugString() {
	std::stringstream buffer;
	buffer << "<InputRuleSpec kind " << InputKindString(kind) << " device " << devicename << " axiscode " << hex << axiscode << " inputcode " << inputcode << ">";
	return buffer.str();
}

uint32_t InputData::match(uint32_t _axiscode) {
	uint32_t maskmatch = MASK_OVERLAP(axiscode, _axiscode, AXISCODE_EVENT_MASK);

	if (maskmatch && _axiscode & AXISCODE_DPAD_MASK) {
		uint32_t dpmatch = MASK_OVERLAP(axiscode, _axiscode, AXISCODE_DPAD_MASK);
		if (dpmatch)
			maskmatch |= dpmatch;
		else
			maskmatch = 0;
	}
	
	if (maskmatch) {
		if (kind == InputKindKeyboard) {
			bool keymatch = MASK_EQUAL(_axiscode, 0, AXISCODE_SDLK_MASK) || MASK_EQUAL(axiscode, _axiscode, AXISCODE_SDLK_MASK);
			if (!keymatch) maskmatch = 0;
		} else {
			bool idmatch = MASK_EQUAL(axiscode, _axiscode, AXISCODE_ID_MASK);
			if (!idmatch) maskmatch = 0;
		}
	}
	
	return maskmatch;
}

string InputData::debugString() {
	std::stringstream buffer;
	buffer << "<InputData kind " << InputKindString(kind) << " inputcode " << hex << inputcode << " axiscode " << AxisCodeString(axiscode) << " str " << strength;
	buffer << ">";
	return buffer.str();
}

// BINARY STORAGE

#define DEFAULTDAT (PROGDIR "controls.obj")
#define F_INPUT_VERSION 0x1

void InputRules::load(const InputRuleSpec &rule) {
	switch(rule.kind) {
		case InputKindVacuum: vacuum = rule.inputcode; break;
		case InputKindRise:   rise = rule.inputcode;   break;
		case InputKindEdge:   edge = rule.inputcode;   break;
		case InputKindTouch:  touch = rule.inputcode;  break;
		case InputKindKeyboard:
			if (!rule.axiscode) { // Shouldn't this be checking ID, not raw axiscode?
				keyboard = rule.inputcode;
			} else {
				key[rule.axiscode] = rule.inputcode;
			}
			break;
		case InputKindAxis:
			controllerFor(rule.devicename)->axis[rule.axiscode] = rule.inputcode;
			break;
		case InputKindHat:
			controllerFor(rule.devicename)->hat[rule.axiscode] = rule.inputcode;
			break;
		case InputKindButton:
			controllerFor(rule.devicename)->button[rule.axiscode] = rule.inputcode;
			break;
		case InputKindSystem:
			system[rule.axiscode] = rule.inputcode; // FIXME: Even stores invalid?
			break;
	}
}

void InputRules::unload(const InputRuleSpec &rule) {
	switch(rule.kind) {
		case InputKindVacuum: vacuum = I_UNUSED; break;
		case InputKindRise:   rise = I_UNUSED;   break;
		case InputKindEdge:   edge = I_UNUSED;   break;
		case InputKindTouch:  touch = I_UNUSED;  break;
		case InputKindKeyboard:
			if (!rule.axiscode) {
				keyboard = I_UNUSED;
			} else {
				key.erase(rule.axiscode);
			}
			break;
		case InputKindAxis:
			controllerFor(rule.devicename)->axis.erase(rule.axiscode);
			break;
		case InputKindHat:
			controllerFor(rule.devicename)->hat.erase(rule.axiscode);
			break;
		case InputKindButton:
			controllerFor(rule.devicename)->button.erase(rule.axiscode);
			break;
		case InputKindSystem:
			system.erase(rule.axiscode);
			break;
	}
}

void InputRules::f_load(FILE *f) {
	uint32_t temp = 0;
	F_READ(temp);
	if (temp != F_INPUT_VERSION) return;
	
	uint32_t rulecount = 0;
	F_READ(rulecount);
	for(int c = 0; c < rulecount; c++) {
		InputRuleSpec rule;
		rule.f_load(f);
		
		load(rule);
	}
}
void InputRuleSpec::f_load(FILE *f) {
	uint32_t temp = 0;
	F_READ(temp);
	kind = (InputKind)temp;
	
	f_reads(f);
	F_READ(axiscode);
	F_READ(inputcode);
}

void InputRules::f_save(FILE *f) {
	uint32_t temp = F_INPUT_VERSION; // Version
	F_WRITE(temp);
	
	vector<InputRuleSpec> d = dump();
	temp = d.size();
	F_WRITE(temp);
	for(int c = 0; c < d.size(); c++)
		d[c].f_save(f);
}
void InputRuleSpec::f_save(FILE *f) {
	uint32_t temp;
	
	temp = kind;
	F_WRITE(temp);
	
	f_writes(f, devicename);
	F_WRITE(axiscode);
	F_WRITE(inputcode);
}
