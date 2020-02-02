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
#include "input_mapping.h"

input_mapper *global_mapper = NULL;

void ditch_setup_controls() {
	if (global_mapper)
		return;
		
	global_mapper = new input_mapper();
	
	global_mapper->addRule(InputRuleSpec(InputKindTouch, INPUTCODE(G_G, I_MOUSE), AXISCODE_RAW_MASK ));
	
	// Movement
	
	global_mapper->addKeyboard(SDL_SCANCODE_A, INPUTCODE(G_DBG, IDBG_MOVE_X), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_W, INPUTCODE(G_DBG, IDBG_MOVE_Z), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_D, INPUTCODE(G_DBG, IDBG_MOVE_X), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_S, INPUTCODE(G_DBG, IDBG_MOVE_Z), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK);
	
	global_mapper->addKeyboard(SDL_SCANCODE_E, INPUTCODE(G_DBG, IDBG_MOVE_Y), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_C, INPUTCODE(G_DBG, IDBG_MOVE_Y), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);

	global_mapper->addKeyboard(SDL_SCANCODE_Z, INPUTCODE(G_DBG, IDBG_ROLL), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_Q, INPUTCODE(G_DBG, IDBG_ROLL), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);

	global_mapper->addKeyboard(SDL_SCANCODE_I, INPUTCODE(G_DBG, IDBG_SHADER1), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_P, INPUTCODE(G_DBG, IDBG_SHADER1), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	
	global_mapper->addKeyboard(SDL_SCANCODE_K, INPUTCODE(G_DBG, IDBG_MUTATE_STRENGTH), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_SEMICOLON, INPUTCODE(G_DBG, IDBG_MUTATE_STRENGTH), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addKeyboard(SDL_SCANCODE_L, INPUTCODE(G_DBG, IDBG_MUTATE_RESET),	AXISCODE_HIGH_MASK);
		
	// Graphics / meta
	
	global_mapper->addKeyboard(SDL_SCANCODE_X, INPUTCODE(G_DBG, IDBG_FLIP), AXISCODE_HIGH_MASK);
	global_mapper->addRule(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_SNAPSHOT), AXISCODE_HIGH_MASK|SDLK_RETURN ));
	global_mapper->addKeyboard(SDL_SCANCODE_BACKSLASH, INPUTCODE(G_DBG, IDBG_PAUSE), AXISCODE_HIGH_MASK);
	global_mapper->addRule(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_STEP), AXISCODE_HIGH_MASK|SDLK_RIGHT ));
	global_mapper->addKeyboard(SDL_SCANCODE_RIGHTBRACKET, INPUTCODE(G_DBG, IDBG_HIDE), AXISCODE_HIGH_MASK);

	for(int c = 0; c < 10; c++)
		global_mapper->addRule(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_NUMBER), AXISCODE_HIGH_MASK| ('0' + c) ));

	// System
	
	global_mapper->addRule(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_TITLE, IT_MENUKEY), AXISCODE_HIGH_MASK));
	global_mapper->addRule(InputRuleSpec(InputKindSystem, INPUTCODE(G_G, I_WILLQUIT), AXISCODE_RAW_MASK|InputSystemQuit ));
	global_mapper->addRule(InputRuleSpec(InputKindSystem, INPUTCODE(G_G, I_RECONTROLLER), AXISCODE_RAW_MASK|InputSystemRecontroller ));
	
	// Joypad

	global_mapper->addJoystick("rightx", INPUTCODE(G_DBG, IDBG_PAN_X), AXISCODE_RAW_MASK|AXISCODE_LOW_MASK);
	global_mapper->addJoystick("righty", INPUTCODE(G_DBG, IDBG_PAN_Y), AXISCODE_RAW_MASK);

	global_mapper->addJoystick("leftx",  INPUTCODE(G_DBG, IDBG_MOVE_X), AXISCODE_RAW_MASK|AXISCODE_LOW_MASK);
	global_mapper->addJoystick("lefty",  INPUTCODE(G_DBG, IDBG_MOVE_Z), AXISCODE_RAW_MASK|AXISCODE_LOW_MASK);

	global_mapper->addJoystick("a", INPUTCODE(G_DBG, IDBG_MUTATE_STRENGTH), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addJoystick("b", INPUTCODE(G_DBG, IDBG_MUTATE_STRENGTH), AXISCODE_LOW_MASK |AXISCODE_ZERO_MASK);
	
	global_mapper->addJoystick("x", INPUTCODE(G_DBG, IDBG_SHADER1), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	global_mapper->addJoystick("y", INPUTCODE(G_DBG, IDBG_SHADER1), AXISCODE_LOW_MASK |AXISCODE_ZERO_MASK);

	global_mapper->addJoystick("leftshoulder", INPUTCODE(G_DBG, IDBG_ROLL), AXISCODE_LOW_MASK |AXISCODE_ZERO_MASK);
	global_mapper->addJoystick("lefttrigger",  INPUTCODE(G_DBG, IDBG_ROLL), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);

	global_mapper->addJoystick("righttrigger",  INPUTCODE(G_DBG, IDBG_MOVE_Y), AXISCODE_LOW_MASK |AXISCODE_ZERO_MASK);
	global_mapper->addJoystick("rightshoulder", INPUTCODE(G_DBG, IDBG_MOVE_Y), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);

	global_mapper->addJoystick("back",  INPUTCODE(G_DBG, IDBG_HIDE), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("start", INPUTCODE(G_DBG, IDBG_PAUSE), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("guide", INPUTCODE(G_DBG, IDBG_SNAPSHOT), AXISCODE_HIGH_MASK);

	global_mapper->addJoystick("dpleft",  INPUTCODE(G_DBG, IDBG_NUMBER), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("dpdown",  INPUTCODE(G_DBG, IDBG_FLIP), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("dpright", INPUTCODE(G_DBG, IDBG_STEP), AXISCODE_HIGH_MASK);
	global_mapper->addJoystick("dpup",    INPUTCODE(G_DBG, IDBG_MUTATE_RESET), AXISCODE_HIGH_MASK);

	global_mapper->insert();
}