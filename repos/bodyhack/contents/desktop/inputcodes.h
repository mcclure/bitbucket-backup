/*
 *  inputcodes.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/16/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _INPUTCODES_H

enum group_code {
	G_GENERAL = 0,
	
	// App specific
	G_PLAYER_L,
	G_PLAYER_R,
};

enum input_code_general {
	I_UNUSED = 0,
	I_VACUUM,		// For debug, control-picker purposes
	
	I_HALT,
	I_RADDOWN,
	I_RADUP,
};

enum input_code_player {	
	IP_UNUSED = 0,
	
	I_MOVE_X,
	I_MOVE_Y,
	
	I_RULE_N,
	I_RULE_S,
	I_RULE_E,
	I_RULE_W,
	
	I_S1, // Shoulder
	I_S2,
};

#endif
