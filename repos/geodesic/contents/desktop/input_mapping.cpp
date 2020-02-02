/*
 *  input_mapping.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 8/30/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#include "input_mapping.h"
#include "input_ent.h"
#include "input.h"
#include "inputcodes.h"
#include "internalfile.h"
#include "json/json.h"

#define LABELTEST 0

typedef hash_map<string, ControllerLabels *>::iterator labels_iter;

input_mapper::input_mapper() : ent(), found_controllers(false) {
	static bool __ever_loaded = false;
	
	stack.push_back(MappingState());
	
	string platform;
	#if defined(TARGET_MAC)
		platform = "Mac OS X";
	#elif defined(TARGET_WINDOWS)
		platform = "Windows"
	#elif defined(TARGET_LINUX)
		platform = "Linux"
	#endif
	
	// FIXME: simplejson is known well-behaved for bad access on NULL values. What about non-NULLs like ints?
	char bindingsfile[FILENAMESIZE];
	internalPath(bindingsfile, "gamecontroller.json");
	// TODO: Check version 
	do {
		Json::Value read_root;
		Json::Reader reader;
		if ( platform.empty() || !reader.parse( filedump(bindingsfile), read_root ))
			break;
		
		const Json::Value &root = read_root;
		const Json::Value db = root["db"];   // Note: const important
		
		registerLabels("default", root["defaultlabels"]);
		
		if ( !root["db"].isArray() )
			break;
			
		for(int c = 0; c < db.size(); c++) {
			const Json::Value controller = db[c];
			const Json::Value name = controller["name"];
			const Json::Value inherit = controller["inherit"];
			const Json::Value platforms = controller["platforms"];
			const Json::Value current_platform = platforms[platform];
			const Json::Value guid = current_platform["guid"];
			const Json::Value mapping = current_platform["mapping"];
			const Json::Value::Members mapping_names = mapping.getMemberNames();
			if (!name.isString() || !guid.isString() || mapping.empty())
				continue;
			
			registerLabels(name.asString(), controller["labels"], inherit.asString());
			
			ostringstream o;
			o << guid.asString() << "," << name.asString() << ",";
			
			for(int m = 0; m < mapping_names.size(); m++) {
				const string &name = mapping_names[m];
				if (name.empty())
					continue;
				o << name << ":" << mapping[name].asString() << ",";
			}
			
			const string &line = o.str();
			// ERR("SENDING: %s\n", line.c_str());
			if (!__ever_loaded)
			SDL_GameControllerAddMapping(line.c_str());
		}
		
		// Fix up labels
		ControllerLabels *defaultLabels = NULL;
		if (labels.count("default"))
			defaultLabels = labels["default"];
		for(labels_iter i = labels.begin(); i != labels.end(); i++) {
			ControllerLabels *l = i->second;
			bool inherits = l->labels.count("inherit");
			ControllerLabels *want = inherits && labels.count( l->labels["inherit"] )
				? labels[ l->labels["inherit"] ] : NULL;
			
#if LABELTEST
			ERR("For %s want to inherit %s\n", l->labels["name"].c_str(), want?want->labels["name"].c_str():"---");
#endif
			if (want)
				l->fallback = want;
			else if (l != defaultLabels) // Prevent loop at default
				l->fallback = defaultLabels;
			
			if (inherits)
				i->second->labels.erase("inherit");
		}
	} while (0);
	
	__ever_loaded = true;
}

void input_mapper::registerLabels(const string &name, const Json::Value &json, const string &inherit) {
	ControllerLabels *l = new ControllerLabels();
	const Json::Value::Members mapping_names = json.getMemberNames();
#if LABELTEST
	ERR("For %s found %d mappings\n", name.c_str(), (int)mapping_names.size());
#endif
	for(int m = 0; m < mapping_names.size(); m++) {
		const string &mapname  = mapping_names[m];
		const string &mapvalue = json[mapname].asString();
#if LABELTEST
		ERR("For '%s' map '%s'\n", mapname.c_str(), mapvalue.c_str());
#endif
		if (!mapname.empty() && !mapvalue.empty())
			l->labels[mapname] = mapvalue;
	}
#if LABELTEST
	l->labels["name"] = name; // Useful for debugging but not in-game?
#endif
	if (!inherit.empty()) // Note: Temporary stash, should be removed before use
		l->labels["inherit"] = inherit;
	labels[name] = l;
}

string input_mapper::label(const string &button, const string &controller) {
	ControllerLabels *l = NULL;
	string startAt = controller.empty() ? firstController : controller;
	if (labels.count(startAt))
		l = labels[startAt];
	while (l) {
#if LABELTEST
		ERR("Checking %s for button %s found? %d\n", l->labels["name"].c_str(), button.c_str(), (int)l->labels.count(button));
#endif
		if (l->labels.count(button))
			return l->labels[button];
		l = l->fallback;
	}
	return "[Unknown]";
}

string input_mapper::label(SDL_Scancode value) {
	char output[2] = {'\0', '\0'};
	output[0] = SDL_GetKeyFromScancode(value);
	return output;
}

// FIXME: Better to just wipe out?
void input_mapper::deassign() {
	for(int c = 0; c < lastSet.size(); c++) {
		InputRuleSpec &spec = lastSet[c];
		InputRules::rules()->unload(spec);
	}
	lastSet.clear();
	firstController.clear();
}

void input_mapper::assign() {
	deassign();
	for(int c = 0; c < state().currentRules.size(); c++)
		InputRules::rules()->load( state().currentRules[c] );
	
	for(int j = 0; j < SDL_NumJoysticks(); j++) {
		SDL_GameController *controller = SDL_GameControllerOpen(j);
		string name = nameForJoystick(j);
		if (firstController.empty())
			firstController = name;
		
		if (controller) {
			found_controllers = true;
			
			for(int c = 0; c < state().currentWishes.size(); c++) {
				InputWishSpec &wish = state().currentWishes[c];
				InputRuleSpec spec;
				
				spec.inputcode = wish.inputcode;
				spec.axiscode = wish.axiscode;
				
				SDL_GameControllerButtonBind result;
				result.bindType = SDL_CONTROLLER_BINDTYPE_NONE;
				
				{
					SDL_GameControllerAxis axis = SDL_GameControllerGetAxisFromString(wish.value.c_str());
					if (axis != SDL_CONTROLLER_AXIS_INVALID)
						result = SDL_GameControllerGetBindForAxis(controller, axis);
				}
				
				if (result.bindType == SDL_CONTROLLER_BINDTYPE_NONE) {
					SDL_GameControllerButton button = SDL_GameControllerGetButtonFromString(wish.value.c_str());
					if (button != SDL_CONTROLLER_BUTTON_INVALID)
						result = SDL_GameControllerGetBindForButton(controller, button);
				}
				
				if (result.bindType != SDL_CONTROLLER_BINDTYPE_NONE) {
					switch (result.bindType) {
						case SDL_CONTROLLER_BINDTYPE_BUTTON:
							spec = InputRuleSpec(InputKindButton, wish.inputcode, wish.axiscode|result.value.button, name);
							break;
						case SDL_CONTROLLER_BINDTYPE_AXIS:
							spec = InputRuleSpec(InputKindAxis, wish.inputcode, wish.axiscode|result.value.axis, name);
							break;
						case SDL_CONTROLLER_BINDTYPE_HAT:
							uint32_t hat = wish.axiscode|result.value.hat.hat;
							if (result.value.hat.hat_mask & (1|4)) hat |= AXISCODE_YHAT_MASK;
							else                         hat |= AXISCODE_XHAT_MASK;
							if ((result.value.hat.hat_mask & (4|8)) && (hat & (AXISCODE_HIGH_MASK|AXISCODE_RAW_MASK))) {
								hat &= ~AXISCODE_HIGH_MASK; // Blank high if it's present
								hat |=  AXISCODE_LOW_MASK;
							}
							spec = InputRuleSpec(InputKindHat, wish.inputcode, hat, name);
							break;
					}
					
					// TODO: Names?
					if (spec.kind != InputKindInvalid)
						InputRules::rules()->load(spec);
					
//					ERR("LOADED FOR %s BIND %d,%d VALUE %s\n", wish.value.c_str(), (int)result.bindType, (int)result.value.button, spec.debugString().c_str());
				}
			}
			SDL_GameControllerClose(controller);
		}
	}
}

MappingState &input_mapper::state() {
	return stack.back();
}

void input_mapper::clear() {
	stack[stack.size()-1] = MappingState();
}

void input_mapper::push(bool inherit) {
	if (inherit)
		stack.push_back(state());
	else
		stack.push_back(MappingState());
}

void input_mapper::pop() {
	if (stack.size() > 1)
		stack.pop_back();
	else
		clear();
}

void input_mapper::addRule(const InputRuleSpec &rule) {
	state().currentRules.push_back(rule);
}

void input_mapper::addKeyboard(SDL_Scancode value, uint32_t inputcode, uint32_t axiscode) {
	addRule(InputRuleSpec(InputKindKeyboard, inputcode, axiscode | SDL_GetKeyFromScancode(value)));
}

void input_mapper::addJoystick(const string &value, uint32_t inputcode, uint32_t axiscode) {
	state().currentWishes.push_back(InputWishSpec(value, inputcode, axiscode));
}

void input_mapper::inserting() { assign(); }

void input_mapper::input(InputData *d) {
	if (d->inputcode == INPUTCODE(G_G, I_RECONTROLLER)) {
		assign();
	}
}
