/*
 *  internalfile.h
 *  Jumpman
 *
 *  Created by Andi McClure on 2/16/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
 
#include <stdio.h>

#define FILENAMESIZE 512

// Abstracts finding files between platforms.
void internalPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);