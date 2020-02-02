/*
 *  freeze.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 12/8/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "freeze.h"

void freeze::f_loadfile(const string &path) {
	FILE *f = fopen(path.c_str(), "rb");
	if (f) {
		f_load(f);
		fclose(f);
	}
}

void freeze::f_savefile(const string &path) {
	FILE *f = fopen(path.c_str(), "wb");
	if (f) {
		f_save(f);
		fclose(f);
	}
}