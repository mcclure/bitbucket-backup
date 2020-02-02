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
#include "internalfile.h"
	
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

input_mapper::input_mapper() : ent(), found_controllers(false) {
	static bool __ever_loaded = false;
	if (!__ever_loaded) {
		char bindingsfile[FILENAMESIZE];
		internalPath(bindingsfile, "gamecontrollerdb.txt");
		SDL_GameControllerAddMappingsFromFile(bindingsfile);

		__ever_loaded = true;
	}
}

void input_mapper::clear() {
	for(int c = 0; c < currentRules.size(); c++) {
		InputRuleSpec &spec = currentRules[c];
		InputRules::rules()->unload(spec);
	}
	currentRules.clear();
}

void input_mapper::assign() {
	clear();
	for(int j = 0; j < SDL_NumJoysticks(); j++) {
		SDL_GameController *controller = SDL_GameControllerOpen(j);
		string name = nameForJoystick(j);
		if (controller) {
			found_controllers = true;
			
			for(int c = 0; c < currentWishes.size(); c++) {
				InputWishSpec &wish = currentWishes[c];
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
					
					ERR("LOADED FOR %s BIND %d,%d VALUE %s\n", wish.value.c_str(), (int)result.bindType, (int)result.value.button, spec.debugString().c_str());
				}
			}
			SDL_GameControllerClose(controller);
		}
	}
}

void input_mapper::addKeyboard(SDL_Scancode  value, uint32_t inputcode, uint32_t axiscode) {
}

void input_mapper::addJoystick(const string &value, uint32_t inputcode, uint32_t axiscode) {
	currentWishes.push_back(InputWishSpec(value, inputcode, axiscode));
}

void input_mapper::inserting() { assign(); }

void input_mapper::input(InputData *) {
}
