// MAIN LOOP

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
 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "kludge.h"

#include "chipmunk.h"
#include "lodepng.h"
#include "internalfile.h"
#include <queue>
#include <string>
#include <vector>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "tinyxml.h"
#include "program.h"
#include "color.h"
#include "glCommon.h"
#include "ftgles.h"
#include <dlfcn.h>

#define COUNT_SAVEDOPTIONS 1
bool savedOptions[COUNT_SAVEDOPTIONS];
bool &optWindow = savedOptions[0];

int fullscreenw = 0, fullscreenh = 0;
int surfacew = 0, surfaceh = 0;

int lastSdlTime = 0, sdlTime = 0, sinceLastFrame = 0;

int ticks = 0;
double aspect = 0;
bool paused = false;
bool gotJoystick = false;
SDL_Joystick *g_joystick = NULL;

void BombBox(string why) {
	// do nothing for now
}

void internalPath(char *dst, const char *fmt, int arg1, int arg2) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);

	char callingpath[FILENAMESIZE];
	PDL_GetCallingPath(callingpath, FILENAMESIZE);
	
	snprintf(dst, FILENAMESIZE, "%s%s%s", callingpath, "Internal/", filename);
	ERR(dst);
}

SDL_Surface *Surface;

#ifdef WIN32
extern "C" 
#endif
GL_API int GL_APIENTRY _dgles_load_library(void *, void *(*)(void *, const char *));

static void *proc_loader(void *h, const char *name)
{
    (void) h;
    return SDL_GL_GetProcAddress(name);
}

void Quit(int code) {
    SDL_Quit();
	
	{
		char optfile[FILENAMESIZE];
		internalPath(optfile, "controls.obj");
		ofstream f;
		f.open(optfile, ios_base::out | ios_base::binary | ios_base::trunc);
		if (!f.fail()) {
			f.write((char *)savedOptions, sizeof(savedOptions));
		}
	}
	
	AboutToQuit();
	
    exit(code);	
}

void QuitWrapper() {
	Quit(0);
}

void sdl_init()
{	
	{ // Load controls and options first, because they are relevant to full screen or no
		bool failure = false;
		char optfile[FILENAMESIZE];
		internalPath(optfile, "controls.obj");

		ifstream f;
		f.open(optfile, ios_base::in | ios_base::binary);
		if (!f.fail()) {
			f.seekg (0, ios::end);
			if (0 != f.tellg()) {
				f.seekg (0, ios::beg);
				f.read((char *)savedOptions, sizeof(savedOptions));
			} else failure = true;
		} else failure = true;
	}

	const SDL_VideoInfo *video = SDL_GetVideoInfo();
	fullscreenw = video->current_w; fullscreenh = video->current_h;
	Surface = SDL_SetVideoMode(fullscreenw,fullscreenh,0,SDL_OPENGL);
	aspect = ((float)fullscreenh) / ((float)fullscreenw);
	surfacew = fullscreenw;
	surfaceh = fullscreenh;
	ERR("Screen resolution is: %d,%d\n", fullscreenw, fullscreenh);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	SDL_AudioSpec *desired;
	desired=(SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
	desired->freq = 44100;
	desired->format = AUDIO_S16SYS;
	desired->channels = 1; // stereo is a stupid gimmick anyway
	desired->samples = 256; // FIXME
	desired->callback = audio_callback;
	desired->userdata = NULL;
	int failure = SDL_OpenAudio(desired, NULL);
	if ( failure < 0 ){ // Does this really need to be treated as fatal?
	  REALERR("Couldn't open audio: %s\n", SDL_GetError());
	  Quit(1);
	} else {
		ERR("Opened audio without error:\n");
	}
	free(desired);
		    
    audio_init();
    
	SDL_PauseAudio(0);
	
	SDL_EnableUNICODE(true);

	// We enable the SDL joysticks in webOS to gain
	// access to the accelerometer
	ERR("Found %d joysticks\n", SDL_NumJoysticks());
	if (SDL_NumJoysticks() > 0) {
		SDL_JoystickEventState(SDL_ENABLE);
		g_joystick = SDL_JoystickOpen(0);
		gotJoystick = true;
	}
}

void initGLBasic(bool reinit)
{
	glClearColor(1.0, 1.0, 1.0, 0.0);

	glEnable (GL_BLEND); 
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	gl2Basic();
    
	display_init(reinit);	
}

void initGL(void)
{
	// webOS port of Jumpcore supports OpenGL ES 2.0 ONLY
	gl2=true;

	initGLBasic(false);
}

// Initializes the application data
int Init(void) 
{
	return true;
}

void Loop(void)
{
	// BEGIN MAIN LOOP
	program_interface();
	int nLoops = 0;
	while ( 1 ) {
		// Todo: It would be nice to be able to dynamically pick a framerate
			sdlTime = SDL_GetTicks();
			sinceLastFrame = sdlTime-lastSdlTime;
			if (sinceLastFrame > TPF) {
				display();
				SDL_GL_SwapBuffers();
				//poll the accelerometer every frame
				float accel_x = (float) SDL_JoystickGetAxis(g_joystick, 0) / 32768.0f;
				float accel_y = -(float) SDL_JoystickGetAxis(g_joystick, 1) / 32768.0f;
				float accel_z = (float) SDL_JoystickGetAxis(g_joystick, 2) / 32768.0f;
				program_eventaccel(accel_x, accel_y, accel_z);
				program_update();
				
				lastSdlTime = sdlTime;
				++nLoops;
				// if ((nLoops%300) == 0) printf ("%d Loops - %d ms/frame\n", nLoops,TPF);
			} else if (sinceLastFrame < TPF-1) {
				SDL_Delay(TPF-1-sinceLastFrame);
			}
		

		// Event loop.
		// Jumpcore reads all events and forwards on to the callbacks the events it thinks are "interesting".
		// Some events get eaten before they reach the callbacks; these would be the "special" keystrokes, and
		// any mouse event that connects with a ControlBase button.
		{ 
			SDL_Event event;
			while ( SDL_PollEvent(&event) ) {
				// Program events:
				  
				if ( event.type == SDL_QUIT ) {
				  Quit();
				}


				// Key events:
				  
				if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
					// ERR("KEY EVENT TYPE %d KEY %d\n", (int)event.type, (int)event.key.keysym.sym); break;
					
					if (event.type == SDL_KEYDOWN && KeyboardControl::focus != NULL) {
						KeyboardControl::focus->key(event.key.keysym.unicode, event.key.keysym.sym);
						
					} else if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE ) {
						BackOut();
						
					} else if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F8 ) {
						
						optWindow = !optWindow;
						// recreate_surface(optWindow);
						
					} else if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F7 ) {
						
						paused = !paused;
						
					} else {
						// program_eventkey(event);
					}
				}
					  
				// Joystick events:
				
				if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP
					|| event.type == SDL_JOYAXISMOTION || event.type == SDL_JOYHATMOTION) {
					// Not handled here, see display loop above
				}
				
				// Mouse events
				  
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					touch_rec touchEvent = touch_rec(event.button.which, cpv(event.button.x, event.button.y));
					list<touch_rec> t_list;
					t_list.push_back(touchEvent);
					program_eventtouch(t_list, touch_down);
				}
				if (event.type == SDL_MOUSEBUTTONUP) {
					touch_rec touchEvent = touch_rec(event.button.which, cpv(event.button.x, event.button.y));
					list<touch_rec> t_list;
					t_list.push_back(touchEvent);
					program_eventtouch(t_list, touch_up);
				}
				if (event.type == SDL_MOUSEMOTION) {
					touch_rec touchEvent = touch_rec(event.button.which, cpv(event.button.x, event.button.y));
					list<touch_rec> t_list;
					t_list.push_back(touchEvent);
					program_eventtouch(t_list, touch_move);
				}
				
				// webOS activate/deactivate card
				if (event.type == SDL_ACTIVEEVENT) {
					switch (event.active.gain) {
						case 0:	program_sleep();
							break;
						case 1: program_wake();
							break;
						default: break;
					}
				}
			}
		}
	}
}

int main(int argc, char** argv)
{
    // Initialize the SDL library with the Video subsystem
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
    atexit(QuitWrapper);
	// do Jumpcore sdl init
	sdl_init();
    
    // start the PDL library
    PDL_Init(0);
    atexit(PDL_Quit);
    
    // Tell it to use OpenGL version 2.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);

#if WIN32
    // Load the desktop OpenGL-ES emulation library
    _dgles_load_library(NULL, proc_loader);
#endif

    // Application specific Initialize of data structures & GL states
    if (Init() == false)
        return -1;

	initGL();
	
	program_init();

	Loop();

    return 0;
}
