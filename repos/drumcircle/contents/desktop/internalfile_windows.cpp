/*
 *  internalfile.cpp
 *  Jumpman
 *
 *  Created by Andi McClure on 2/16/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "internalfile.h"

// Gets an fopen-compatible path for a file located in the package resources.
// dst assumed at least 512 bytes long.
void internalPath(char *dst, const char *fmt, int arg1, int arg2) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);
    snprintf(dst, FILENAMESIZE, "Internal/%s", filename); // LOOK HOW SIMPLE THAT WAS
}

void userPath(char *dst, const char *fmt, int arg1, int arg2) {
	snprintf(dst, FILENAMESIZE, fmt, arg1, arg2);
}
