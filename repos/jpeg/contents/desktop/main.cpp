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
#include "internalfile.h"
#include "FTGL/ftgles.h"
#include <queue>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fstream>
#include "tinyxml.h"
#include "program.h"
#include "color.h"
#include "glCommon.h"

#define GL_FUNCTIONALITY_TEST 1
#define FORCE_GL1 0
#define IN_A_WINDOW SELF_EDIT

// The program lugs around a file named control.obj inside it that stores certain program settings.
// In the typewriter demo the only thing stored in it is the "full screen mode" setting, but you can add more here:
#define COUNT_SAVEDOPTIONS 1
bool savedOptions[COUNT_SAVEDOPTIONS];
bool &optWindow = savedOptions[0];

#if defined(__APPLE__) && !IN_A_WINDOW
// Support for alt-tab-equals-minimize feature
bool minimizeWant = false, minimizeAlreadyPaused = false;
#endif

int fullscreenw = 0, fullscreenh = 0;
int surfacew = 0, surfaceh = 0;

int lastSdlTime = 0, sdlTime = 0, sinceLastFrame = 0;

// Global framecount clock
int ticks = 0;
double aspect;
#if SELF_EDIT
bool paused = true;
#else
bool paused = false;
#endif
void pause(bool p) { paused = p; }

bool gotJoystick = false;

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

void sdl_surface_init(bool in_a_window)
{
	SDL_Surface *surface;
#if IN_A_WINDOW
	in_a_window = true;
#endif
	
	if (in_a_window)
		surface = SDL_SetVideoMode(
			600.0*fullscreenw/fullscreenh, 600, 0, // So for example this will be 800x600 on a 1024x768 screen.
			SDL_OPENGL);
	else
		surface = SDL_SetVideoMode(
				fullscreenw, fullscreenh, 0, SDL_FULLSCREEN | 
				SDL_OPENGL);
			
  if (surface  == NULL ) {
    REALERR("Unable to create OpenGL screen: %s\n", SDL_GetError());
	Quit(2);
  }
  
  aspect = double(surface->h)/surface->w;
  surfacew = surface->w; surfaceh = surface->h;
  
  ERR("Window size %dx%d ratio %lf\n", surface->w, surface->h, aspect);
  
#if VSYNC_FIX
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // BEFORE
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
#ifdef __APPLE__
	GLint swapInterval = 1; // SWAP_CONTROL has a bug on macs, so it doesn't take effect unless we say:
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);
#endif
#endif
  SDL_WM_SetCaption(NAME_OF_THIS_PROGRAM, NULL);
}

void initGLBasic(bool reinit);
void recreate_surface(bool in_a_window) {
	ERR("Recreating surface\n");
	sdl_surface_init(in_a_window);
	initGLBasic(true);
	for(hash_map<void *, texture_slice *>::iterator b = texture_slice::reconstruct_registry.begin(); b != texture_slice::reconstruct_registry.end(); b++) {
		if ((*b).second)
			(*b).second->construct();
	}
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
	ERR("Screen resolution is: %d,%d\n", fullscreenw, fullscreenh);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	sdl_surface_init(optWindow);
	
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
	}
	free(desired);
		    
    audio_init();
    
	SDL_PauseAudio(0);
	
	SDL_EnableUNICODE(true);
	
	ERR("Found %d joysticks\n", SDL_NumJoysticks());
	if (SDL_NumJoysticks() > 0) {
		SDL_JoystickEventState(SDL_ENABLE);
		SDL_JoystickOpen(0);
		gotJoystick = true;
	}
}

void initGLBasic(bool reinit)
{
	glClearColor(1.0, 1.0, 1.0, 0.0);

	glPointSize(20.0);
		
	glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	gl2Basic();
    
	display_init(reinit);	
}

void initGL(void)
{
    gl2 = !FORCE_GL1 && GLEE_VERSION_2_0; // TODO: Fallback for gl1
    
	initGLBasic(false);
}

void Loop() {
	program_interface();
	
  while ( 1 ) {
	{ // Todo: It would be nice to be able to dynamically pick a framerate
		sdlTime = SDL_GetTicks();
		sinceLastFrame = sdlTime-lastSdlTime;
		if (sinceLastFrame > TPF) {
			display();
            SDL_GL_SwapBuffers();
            program_update();

#if defined(__APPLE__) && !IN_A_WINDOW
			if (minimizeWant) {
				minimizeWant = false;
				SDL_WM_IconifyWindow();
			}		
#endif
			
			lastSdlTime = sdlTime;
		} else if (sinceLastFrame < TPF-1) {
			SDL_Delay(TPF-1-sinceLastFrame);
		}
	}

	// Event loop.
	// Jumpcore reads all events and forwards on to the callbacks the events it thinks are "interesting".
	// Some events get eaten before they reach the callbacks; these would be the "special" keystrokes, and
	// any mouse event that connects with a ControlBase button.
	{ SDL_Event event;
      while ( SDL_PollEvent(&event) ) {
		// Program events:
		  
        if ( event.type == SDL_QUIT ) {
          Quit();
        }

#if defined(__APPLE__) && !IN_A_WINDOW
		if (event.type == SDL_ACTIVEEVENT && event.active.gain && event.active.state == SDL_APPACTIVE && !optWindow) {
			recreate_surface(false);
			if (!minimizeAlreadyPaused)
				pause(false);
		}
#endif

		// Key events:
		  
		if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
//			ERR("KEY EVENT TYPE %d KEY %d\n", (int)event.type, (int)event.key.keysym.sym); break;

#if defined(__APPLE__)
			SDLMod mods = SDL_GetModState(); // OS X has some issues we have to work around.
#if !IN_A_WINDOW
			if (!optWindow && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB && (mods & KMOD_LMETA)) { // We have to simulate alt-tab working
				recreate_surface(true); // leave fullscreen, become a window
				minimizeAlreadyPaused = paused;
				if (!minimizeAlreadyPaused)
					pause(true);
				minimizeWant = true;
			} else
#endif
			// Also, we need to recognize option-up and option-down for when there is no mouse wheel.
			// Note: I haven't tested this recently... also it would be nice if it somehow pretended to be at the mouse location
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_UP && (mods & KMOD_LALT || mods & KMOD_RALT)) {
				SDL_Event fakeEvent; fakeEvent.type = SDL_MOUSEBUTTONDOWN; fakeEvent.button.button = 4;
				fakeEvent.button.which = 0; fakeEvent.button.x = surfacew/2; fakeEvent.button.y = surfaceh/2; fakeEvent.button.state = SDL_PRESSED;
				program_eventmouse(fakeEvent);
			} else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DOWN && (mods & KMOD_LALT || mods & KMOD_RALT)) {
				SDL_Event fakeEvent; fakeEvent.type = SDL_MOUSEBUTTONDOWN; fakeEvent.button.button = 4;
				fakeEvent.button.which = 0; fakeEvent.button.x = surfacew/2; fakeEvent.button.y = surfaceh/2; fakeEvent.button.state = SDL_PRESSED;
				program_eventmouse(fakeEvent);
			} else 
#endif
			if (event.type == SDL_KEYDOWN && KeyboardControl::focus != NULL) {
				KeyboardControl::focus->key(event.key.keysym.unicode, event.key.keysym.sym);
				
			} else if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE ) {
				BackOut();
				
			} else if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F8 ) {
				
				optWindow = !optWindow;
				recreate_surface(optWindow);
				
			} else if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F7 ) {
				
				paused = !paused;
				
			} else
				program_eventkey(event);
		}
		  
		// Joystick events:
		
		if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP
			|| event.type == SDL_JOYAXISMOTION || event.type == SDL_JOYHATMOTION) {
			program_eventjoy(event);
		}
		
		// Mouse events
		  
		if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
						
			program_eventmouse(event);
		}
	}
    }
  }
}

int main(int argc, char*argv[]) 
{
	{
		time_t rawtime;
		time ( &rawtime );
		srandom(rawtime);
	}
	
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 ) {
	  REALERR("Unable to initialize SDL: %s\n", SDL_GetError());
	  return 1; // For some reason Windows Vista seems to freak out if I call exit() here?
    }
	
	sdl_init();

#if GL_FUNCTIONALITY_TEST
    REALERR("\tGLEE:\n");
    REALERR("VERSION_1_2: %s\n", (GLEE_VERSION_1_2?"Y":"N"));
    REALERR("VERSION_1_3: %s\n", (GLEE_VERSION_1_3?"Y":"N"));
    REALERR("VERSION_1_4: %s\n", (GLEE_VERSION_1_4?"Y":"N"));
    REALERR("VERSION_1_5: %s\n", (GLEE_VERSION_1_5?"Y":"N"));
    REALERR("VERSION_2_0: %s\n", (GLEE_VERSION_2_0?"Y":"N"));
    REALERR("VERSION_2_1: %s\n", (GLEE_VERSION_2_1?"Y":"N"));
    REALERR("VERSION_3_0: %s\n", (GLEE_VERSION_3_0?"Y":"N"));
    REALERR("ARB_vertex_shader: %s\n", (GLEE_ARB_vertex_shader?"Y":"N"));
    REALERR("ARB_fragment_shader: %s\n", (GLEE_ARB_fragment_shader?"Y":"N"));
    REALERR("\n\tglGetString:\n");
    REALERR("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
    REALERR("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    REALERR("GL_VERSION: %s\n", glGetString(GL_VERSION));
    REALERR("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    REALERR("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
#endif
    
	initGL();
	
	program_init();
	
	Loop();
	
	return 0;
}