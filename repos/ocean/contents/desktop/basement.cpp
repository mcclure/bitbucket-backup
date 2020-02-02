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
	
	rules->load(InputRuleSpec(InputKindTouch, INPUTCODE(G_G, I_MOUSE), AXISCODE_RAW_MASK ));
	
#if DBG_CONTROLS
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_P1), AXISCODE_LOW_MASK|'i' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_P1), AXISCODE_HIGH_MASK|'p' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_MOVE_X), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|'a' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_MOVE_Y), AXISCODE_LOW_MASK|AXISCODE_ZERO_MASK|'w' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_MOVE_X), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|'d' ));
	rules->load(InputRuleSpec(InputKindKeyboard, INPUTCODE(G_DBG, IDBG_MOVE_Y), AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK|'s' ));
#endif
	
	rules->load(InputRuleSpec(InputKindSystem, INPUTCODE(G_G, I_WILLQUIT), AXISCODE_RAW_MASK|InputSystemQuit ));
}