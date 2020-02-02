#include <stdlib.h>
#include <time.h>
#include <SDL/SDL.h>
#include "settings.h"
#include "paths.h"
#include "error.h"
#include "graphics.h"
#include "sound.h"
#include "game.h"

SDL_Surface* screenSurface;
GLuint screenTex;

void initSystem(void){
	loadSettings(PATH_SETTINGS);
	srand(time(NULL));
	
	// Creating the display
	if(SDL_Init(SDL_INIT_VIDEO) == -1) errorarg(ERROR_SDLINIT, SDL_GetError());
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	screenSurface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SCREEN_FLAGS);
	if(screenSurface == NULL) errorarg(ERROR_VIDEOINIT, SDL_GetError());
	SDL_WM_SetCaption(SCREEN_CAPTION, SCREEN_CAPTION);
	// Setting up a 2D view
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(0.375, 0.375, 0);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_SCISSOR_TEST);
	// Primitives alpha-blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	initSound();
	initFont(PATH_FONT);
	
	// Creating the framebuffer
	screenTex = createTexture(SCREEN_WIDTH/SCREEN_ZOOM, SCREEN_HEIGHT/SCREEN_ZOOM);
	
	initFramebuffer();
	activateFramebuffer();
	attachFramebufferTexture(screenTex);
	validateFramebuffer();
	deactivateFramebuffer();
}

void cleanupSystem(void){
	deleteTexture(screenTex);
	cleanupFramebuffer();
	cleanupFont();
	cleanupSound();
	SDL_Quit();
}

#ifdef WIN32
#undef main
#endif // WIN32
int main(int argc, char* argv[]){
	initSystem();
	game(screenTex);
	cleanupSystem();
	return 0;
}
