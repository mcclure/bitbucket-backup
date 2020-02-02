// Cross-platform "internal file" code

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2010 Andi McClure
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
