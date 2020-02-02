/*
 *  basement.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/24/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */
 
// Ugly stuff I don't want in the code

#include "basement.h"
#include "input.h"
#include "inputcodes.h"


void ditch_setup_controls() {
	InputRules *rules = InputRules::rules();
	
	if (SDL_NumJoysticks()) {
		const char *name = SDL_JoystickName(0);
		ERR("Found joystick %s\n", name);
	
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_ROT_X), AXISCODE_LOW_MASK|7 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_ROT_Y), AXISCODE_LOW_MASK|4 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_ROT_X), AXISCODE_HIGH_MASK|5 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_ROT_Y), AXISCODE_HIGH_MASK|6 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_LASER), AXISCODE_HIGH_MASK|0xc , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_LASER), AXISCODE_HIGH_MASK|0xd , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_LASER), AXISCODE_HIGH_MASK|0xe , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_G, I_LASER), AXISCODE_HIGH_MASK|0xf , name));
	}
	
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_G, I_ROT_X), AXISCODE_LOW_MASK|SDLK_LEFT ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_G, I_ROT_Y), AXISCODE_LOW_MASK|SDLK_UP ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_G, I_ROT_X), AXISCODE_HIGH_MASK|SDLK_RIGHT ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_G, I_ROT_Y), AXISCODE_HIGH_MASK|SDLK_DOWN ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_G, I_LASER), AXISCODE_HIGH_MASK|SDLK_SPACE ));
	rules->load(InputRuleSpec(InputKindSystem, INPUTCODE(G_G, I_WILLQUIT), AXISCODE_RAW_MASK|InputSystemQuit ));
}