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
	G_TITLE,
	G_T = G_TITLE,
	
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

enum input_code_debug {
	IDBG_UNUSED = 0,
	
	IDBG_MOVE_X,
	IDBG_MOVE_Y,
	IDBG_MOVE_Z,
	IDBG_SHADER1,
	IDBG_MUTATE_STRENGTH,
	IDBG_MUTATE_RESET,
	IDBG_PAN_X,
	IDBG_PAN_Y,
	IDBG_ROLL,
	IDBG_FLIP,
	IDBG_SNAPSHOT,
	IDBG_PAUSE,
	IDBG_STEP,
	IDBG_HIDE,
	IDBG_NUMBER,
};

enum input_code_title {
	IT_UNUSED = 0,
	IT_MENUKEY, // Keyboard vacuum
};

#endif
