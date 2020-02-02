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
	G_G = 0,
	
	// App specific
	
};

enum input_code_general {
	I_UNUSED = 0,
	I_VACUUM,		// For debug, control-picker purposes
	
	// App specific
	I_ROT_X,
	I_ROT_Y,
	I_LASER,
	
	I_WILLQUIT,
};

#endif
