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
	G_G = G_GENERAL,
	G_DEBUG,
	G_DBG = G_DEBUG,
	
	// App specific
	
};

enum input_code_general {
	I_UNUSED = 0,
	I_VACUUM,		// For debug, control-picker purposes
	
	// App specific
	I_MOUSE,
	
	I_RECONTROLLER,
	I_BACKOUT,
	I_WILLQUIT,
};

#endif
