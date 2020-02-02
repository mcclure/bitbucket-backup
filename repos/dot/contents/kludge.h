// Platform-specific kludges

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andi McClure
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

#ifndef _JUMPCORE_KLUDGES
#define _JUMPCORE_KLUDGES

// Two features that seem to work, but I'm not sure I trust. Feel free to turn them on, although
// I suggest that IF you enable either one, then you should ALSO enable the other.
	// If defined true, this will prevent vertical tearing in exchange for a potential drop in framerate.
	#define VSYNC_FIX 0
	// If defined true, then Chipmunk will be less efficient-- *but* if the framerate ever drops, then
	// the physics timestep will adjust to the drop elegantly instead of everything "slowing down".
	#define DYNAMIC_FRAMERATE 0

#ifdef __APPLE__
	#include "OpenGL/gl.h"
	#include "OpenGL/glu.h"
//	#include <GLUT/glut.h>
	#include "sdl.h"
	#include "SDL_endian.h"
	#include "OpenGL/OpenGL.h"
#else

#ifdef WINDOWS
#include <windows.h>
#define _STDCALL_SUPPORTED
#endif

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#include "SDL/SDL.h"
#include "SDL/SDL_endian.h"

#endif

#define RANDOM_MAX ((1<<31)-1)

#ifdef WINDOWS

#define srandom srand
inline long random() {
	unsigned long r1 = rand();
	unsigned long r2 = rand();
	return ((r1 << 16)|r2) & RANDOM_MAX;
}

#endif

#ifdef LINUX
#include <netinet/in.h> 
#endif

#if 1 && SELF_EDIT
#define ERR(...) fprintf (stderr, __VA_ARGS__)
#else
#define ERR(...)
#endif
#define REALERR(...) fprintf (stderr, __VA_ARGS__)

#ifdef WINDOWS
// Is this even necessary? On my system mingw freaks out when I try to link in htonl.
// But windows only ever has one endianness, so that's easy enough to fix... -- mcc
#define htonl not_htonl
#define ntohl not_htonl

inline long not_htonl (long i) {
	long r;
    char *p = (char *)&r;
	char *c = (char *)&i;
	p[0] = c[3];
	p[1] = c[2];
	p[2] = c[1];
	p[3] = c[0];
	return *((long *)p);
}
#endif
#ifdef LINUX
#include <netinet/in.h>
#endif

#define NAMETRACK 1


#endif /* _JUMPCORE_KLUDGES */