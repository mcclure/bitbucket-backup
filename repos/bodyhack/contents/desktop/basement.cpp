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
	
		rules->load(InputRuleSpec(InputKindAxis, INPUTCODE(G_PLAYER_L, I_MOVE_X), AXISCODE_RAW_MASK|0 , name));
		rules->load(InputRuleSpec(InputKindAxis, INPUTCODE(G_PLAYER_L, I_MOVE_Y), AXISCODE_RAW_MASK|1 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_L, I_RULE_W), AXISCODE_HIGH_MASK|7 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_L, I_RULE_N), AXISCODE_HIGH_MASK|4 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_L, I_RULE_E), AXISCODE_HIGH_MASK|5 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_L, I_RULE_S), AXISCODE_HIGH_MASK|6 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_L, I_S1), AXISCODE_HIGH_MASK|10 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_L, I_S2), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|8 , name));
		
		rules->load(InputRuleSpec(InputKindAxis, INPUTCODE(G_PLAYER_R, I_MOVE_X), AXISCODE_RAW_MASK|2 , name));
		rules->load(InputRuleSpec(InputKindAxis, INPUTCODE(G_PLAYER_R, I_MOVE_Y), AXISCODE_RAW_MASK|3 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_R, I_RULE_W), AXISCODE_HIGH_MASK|15 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_R, I_RULE_N), AXISCODE_HIGH_MASK|12 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_R, I_RULE_E), AXISCODE_HIGH_MASK|13 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_R, I_RULE_S), AXISCODE_HIGH_MASK|14 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_R, I_S1), AXISCODE_HIGH_MASK|11 , name));
		rules->load(InputRuleSpec(InputKindButton, INPUTCODE(G_PLAYER_R, I_S2), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|9 , name));
	}
	
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_MOVE_X), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|'a' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_MOVE_Y), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|'w' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_MOVE_X), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|'d' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_MOVE_Y), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|'s' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_RULE_W), AXISCODE_HIGH_MASK|'7' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_RULE_N), AXISCODE_HIGH_MASK|'8' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_RULE_E), AXISCODE_HIGH_MASK|'9' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_L, I_RULE_S), AXISCODE_HIGH_MASK|'0' ));
	
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_MOVE_X), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|SDLK_LEFT ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_MOVE_Y), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|SDLK_UP ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_MOVE_X), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|SDLK_RIGHT ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_MOVE_Y), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|SDLK_DOWN ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_RULE_W), AXISCODE_HIGH_MASK|'z' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_RULE_N), AXISCODE_HIGH_MASK|'x' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_RULE_E), AXISCODE_HIGH_MASK|'c' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_PLAYER_R, I_RULE_S), AXISCODE_HIGH_MASK|'v' ));
}