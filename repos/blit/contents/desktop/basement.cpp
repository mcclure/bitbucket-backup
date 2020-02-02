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
#include "input_ent.h"

input_mapper *global_mapper = NULL;

void ditch_setup_controls() {
	InputRules *rules = InputRules::rules();

	if (global_mapper)
		return;
		
	global_mapper = new input_mapper();

	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_MOVE_X), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|SDLK_RIGHT ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_MOVE_Y), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|SDLK_DOWN ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_MOVE_X), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|SDLK_LEFT ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_MOVE_Y), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|SDLK_UP ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_COMPLETE), AXISCODE_HIGH_MASK|SDLK_SPACE ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_CONFIRM), AXISCODE_HIGH_MASK|SDLK_y ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_CANCEL), AXISCODE_HIGH_MASK|SDLK_n ));
	
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_MUTE), AXISCODE_HIGH_MASK|SDLK_m ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_GAM, IGAM_DEATHMETAL), AXISCODE_HIGH_MASK|SDLK_b ));
	
	global_mapper->addJoystick("leftx",  INPUTCODE(G_GAM, IGAM_MOVE_X), AXISCODE_RAW_MASK);
	global_mapper->addJoystick("lefty",  INPUTCODE(G_GAM, IGAM_MOVE_Y), AXISCODE_RAW_MASK);
	global_mapper->addJoystick("start", INPUTCODE(G_GAM, IGAM_COMPLETE), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("x", INPUTCODE(G_GAM, IGAM_CONFIRM), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("a", INPUTCODE(G_GAM, IGAM_CANCEL), AXISCODE_HIGH_MASK);

	global_mapper->insert();
}