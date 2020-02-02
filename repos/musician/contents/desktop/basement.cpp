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
	InputRules::rules()->load( InputRuleSpec(InputKindKeyboard, INPUTCODE(G_INSTRUMENT, I_VACUUM) ) );
}