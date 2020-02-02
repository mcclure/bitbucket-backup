/*
 *  basement.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/24/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifdef SELF_EDIT
#define DBG_CONTROLS 1
#endif

struct input_mapper;
extern input_mapper *global_mapper;

void ditch_setup_controls();