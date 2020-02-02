/*
 *  game.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/27/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "game.h"
#include "display_ent.h"
#include "test_ent.h"
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "display_ent.h"
#include "glCommonMatrix.h"
#include "util_pile.h"
#include "plaidext.h"

// Put everything here
struct runner : public ent {
	runner() : ent() {}
};

void game_init() {
	int mode = 1;
	cheatfile_load(mode, "mode.txt");
	
	int mute = 0;
	cheatfile_load(mute, "mute.txt");
	if (mute) audio->volume(0);
	
#ifndef SELF_EDIT
	SDL_ShowCursor(0);
#endif

	switch (mode) {
		case 1:
			ditch_setup_controls();
			(new runner())->insert();
			break;
		case -1: {
			(new input_vacuum_ent(InputKindEdge))->insert();
		} break;
	};
}