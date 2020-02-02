// Platform-specific kludges. Always include as early as possible

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

#ifndef _JUMPCORE_KLUDGES
#define _JUMPCORE_KLUDGES

// Two features that seem to work, but I'm not sure I trust. Feel free to turn them on, although
// I suggest that IF you enable either one, then you should ALSO enable the other.
	// If defined true, this will prevent vertical tearing in exchange for a potential drop in framerate.
	#define VSYNC_FIX 1
	// If defined true, then Chipmunk will be less efficient-- *but* if the framerate ever drops, then
	// the physics timestep will adjust to the drop elegantly instead of everything "slowing down".
	#define DYNAMIC_FRAMERATE 0

// Note: TARGET_IPHONE must be defined in the project file

#if defined(TARGET_IPHONE) || defined(TARGET_ANDROID)
#define TARGET_MOBILE
#else
#define TARGET_DESKTOP
#endif

// __APPLE__ is true on iPhone, so check TARGET_IPHONE first:
#ifdef TARGET_MOBILE

#define OPENGL_ES

#ifdef TARGET_IPHONE 
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#else // (TARGET_ANDROID)
#include <GLES/gl.h>
#include <GLES/glext.h> 
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h> 

#define fmodl fmod
#include <sys/endian.h>
#endif

#include "glu.h"

// Added for Jumpcore 2.0, unsure about this:
#define Uint16 uint16_t // Necessary?
#define SDLKey uint32_t // Correct?

#else

// If TARGET_IPHONE is false, then __APPLE__ means we are on mac:
#ifdef __APPLE__
    #include "GLee.h"
	#include "OpenGL/gl.h"
	#include "OpenGL/glu.h"
//	#include <GLUT/glut.h>
	#include "sdl.h"
	#include "SDL_endian.h"
	#include "OpenGL/OpenGL.h"
#else

// If neither TARGET_IPHONE nor __APPLE__, then we are on either Windows or Linux:

// Windows specific:
#ifdef WINDOWS
#include <windows.h>
#include <limits.h>
#define _STDCALL_SUPPORTED
#endif

// Linux specific
#ifdef LINUX
#include <limits.h>
#endif

// Common to Windows+Linux:

// For OpenGL 1.0:
//	#include <GL/gl.h>
//	#include <GL/glext.h>

// For OpenGL 2.0:
#include "GLee.h"

#include <GL/glu.h>

#include "SDL/SDL.h"
#include "SDL/SDL_endian.h"

#endif

#endif

#define RANDOM_MAX ((((unsigned int)1)<<31)-1)

#ifdef WINDOWS

#define srandom srand
inline long random() {
	unsigned long r1 = rand();
	unsigned long r2 = rand();
	return ((r1 << 16)|r2) & RANDOM_MAX;
}

#endif

#ifdef LINUX
// ntohl lives here
#include <netinet/in.h> 
#endif

#if SELF_EDIT
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

using namespace std;

// Real quick, some magical incantations to allow the use of hash_map:
// Switch 0 to 1 on this next line if you are using crystax ndk rather than Google ndk
#if 0 || !defined(TARGET_ANDROID)
#include <ext/hash_map>
using namespace ::__gnu_cxx;
namespace __gnu_cxx {                                                                                             
	template<> struct hash< std::string > // Allow STL strings with hash_map
	{ size_t operator()( const std::string& x ) const { return hash< const char* >()( x.c_str() ); } };          
	template<> struct hash< void * > // Allow pointers with hash_map               
	{ size_t operator()( const void * x ) const { return hash< unsigned int >()( (unsigned int)x ); } };          
}          
#else
#include <hash_map>
// FIXME: hash functions for android
#endif

// Everyone uses some form of pthreads
#include <pthread.h>

#endif /* _JUMPCORE_KLUDGES */
