/*
 *  internalfile.cpp
 *  Jumpman
 *
 *  Created by Andi McClure on 2/16/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "internalfile.h"
#include "CoreFoundation/CoreFoundation.h"

// Gets an fopen-compatible path for a file located in the package resources.
// dst assumed at least 512 bytes long.
void internalPath(char *dst, const char *fmt, int arg1, int arg2) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);

	CFURLRef url = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFStringCreateWithCString(kCFAllocatorDefault, filename, kCFStringEncodingMacRoman), NULL, NULL );
	if (!url) {
		printf ("NOT FOUND %s\n", filename);
		dst[0] = '\0';
		return;
	}
	CFStringRef cfs = CFURLCopyFileSystemPath ( url, kCFURLPOSIXPathStyle  ); // Originally CFURLGetString

	CFStringGetCString(cfs,dst,FILENAMESIZE,kCFStringEncodingASCII);
}
