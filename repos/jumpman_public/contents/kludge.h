/*
 *  kludge.h
 *  Jumpman
 *
 *  Created by Andi McClure on 2/17/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

// Platform-specific kludges

#ifdef __APPLE__
	#include "OpenGL/gl.h"
	#include "OpenGL/glu.h"
//	#include <GLUT/glut.h>
	#include "sdl.h"
	#include "SDL_endian.h"
#else

#ifdef WINDOWS
#include <windows.h>
#define _STDCALL_SUPPORTED
//#include "glut.h"
#endif

	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GL/glu.h>
#ifndef WINDOWS
//	#include <GL/glut.h>
#endif

#include "SDL/SDL.h"
#include "SDL/SDL_endian.h"

#endif

#ifdef WINDOWS
#define msleep(n) Sleep(n)
#else
#define msleep(n) usleep((n)*1000)
#endif

#ifdef LINUX
#include <netinet/in.h> 
#endif
