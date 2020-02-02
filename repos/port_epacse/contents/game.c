#include "game.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "settings.h"
#include "paths.h"
#include "error.h"
#include "utils.h"
#include "graphics.h"
#include "input.h"
#include "sound.h"
#include "level.h"
#include "dot.h"

typedef enum GameState {
	STATE_NONE = 0,
	STATE_SPLASH,
	STATE_GAME,
	STATE_SECRET
} GameState;

int win = 0, starting = 0;

GameState state = STATE_NONE;
int splashPassing = 180;
GLuint splash, win1, win2;

static unsigned char running;

static int framesToRender, framesRendered;

static Input input;

static void initGame(void){
	initDot();
	initLevels(PATH_LEVELINIT);
	input.left = input.right = input.crouch = input.up = 0;
	running = 1;
	splash = createTextureFromImage(PATH_LEVELBASE"/splash.png");
	win1 = createTextureFromImage(PATH_LEVELBASE"/win1.png");
	win2 = createTextureFromImage(PATH_LEVELBASE"/win2.png");
	state = STATE_SPLASH;
}

static void cleanupGame(void){
	deleteTexture(win2);
	deleteTexture(win1);
	deleteTexture(splash);
	cleanupLevels();
	cleanupDot();
}

static void handleInput(void){
	SDL_Event event;
	
	switch(state){
		case STATE_SPLASH:
			if(splashPassing < 180) break;
			while(SDL_PollEvent(&event)){
				switch(event.type){
					case SDL_QUIT: running = 0; break;
					case SDL_KEYDOWN:
						--splashPassing;
					default:;
				}
			}
			break;
		case STATE_GAME:
		case STATE_SECRET:
			while(SDL_PollEvent(&event)){
				switch(event.type){
					case SDL_QUIT: running = 0; break;
					case SDL_KEYDOWN:
							 if(event.key.keysym.sym == KEY_QUIT)   running = 0;
						else if(event.key.keysym.sym == KEY_LEFT)   input.left = 1;
						else if(event.key.keysym.sym == KEY_RIGHT)  input.right = 1;
						else if(event.key.keysym.sym == KEY_UP)     input.up = 1;
						else if(event.key.keysym.sym == KEY_CROUCH) input.crouch = 1;
						else if(event.key.keysym.sym == SDLK_f)     toggleFPS();
						break;
					case SDL_KEYUP:
							 if(event.key.keysym.sym == KEY_LEFT)   input.left = 0;
						else if(event.key.keysym.sym == KEY_RIGHT)  input.right = 0;
						else if(event.key.keysym.sym == KEY_UP)     input.up = 0;
						else if(event.key.keysym.sym == KEY_CROUCH) input.crouch = 0;
						break;
					default:;
				}
			}
			break;
		default:;
	}
}

static void update(unsigned char partial){
	if((state == STATE_SPLASH || win > 0 || starting == 1) && splashPassing < 180) --splashPassing;
	while(state == STATE_SPLASH){
		if(splashPassing < 0){ state = STATE_GAME; starting = 1; splashPassing = 179; break; }
		return;
	}
	if(state != STATE_SECRET) updateLevel(&input, partial);
	updateSound();
}

static void renderFramerate(void){
	static char text[] = "FPS:XXXX";
	static int lastTicks = -1;
	static int framesRendered = 0;
	static float fps = 60.0;
	
	if(lastTicks == -1) lastTicks = SDL_GetTicks();
	++framesRendered;
	if(SDL_GetTicks() >= lastTicks + 1000){
		lastTicks += 1000;
		fps = framesRendered;
		framesRendered = 0;
	}
	
	glColor3ub(150, 150, 0);
	snprintf(text, 8, "fps:%d", (int)fps);
	printText(2, 2, text);
	glColor3ub(255, 255, 255);
}

static void redraw(GLuint screenTex){
	glClear(GL_COLOR_BUFFER_BIT);
	
	glScissor(0, 0, SCREEN_WIDTH/SCREEN_ZOOM, SCREEN_HEIGHT/SCREEN_ZOOM);
	activateFramebuffer();
	glClear(GL_COLOR_BUFFER_BIT);
	switch(state){
		case STATE_SPLASH:
			drawTexture(0, 0, 200, 150, splash);
			if(splashPassing < 180){
				glColor4ub(0, 0, 0, (float)(180-splashPassing)/180.0*255.0);
				drawRect(0, 0, 200, 150);
				glColor4ub(255, 255, 255, 255);
			}
			break;
		case STATE_GAME:
			renderLevel();
			if(starting){
				if(splashPassing <= 0) starting = 0;
				glColor4ub(0, 0, 0, (float)(splashPassing)/180.0*255.0);
				drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
				glColor4ub(255, 255, 255, 255);
			}
			if(win == 1 || win == 10 || win == 100 || win == 1000 || win == 10000){
				if(win == 1 && splashPassing == 0){
					win = 10;
					splashPassing = 179;
				}
				if(win == 10 && splashPassing == 0){
					win = 100;
					splashPassing = 179;
				}
				if(win == 100 && splashPassing == 0){
					win = 1000;
					splashPassing = 179;
				}
				if(win == 1000 && splashPassing == 0){
					win = 10000;
					splashPassing = 179;
				}
				if(win == 10000 && splashPassing == 0){
					running = 0;
				}
				if(win == 10){
					glColor4ub(255, 255, 255, (float)(180-splashPassing)/180.0*255.0);
					drawTexture(22, 66, 134, 20, win1);
					glColor4ub(255, 255, 255, 255);
				} else if(win > 10){
					drawTexture(22, 66, 134, 20, win1);
				}
				if(win == 10000){
					glColor4ub(0, 0, 0, (float)(180-splashPassing)/180.0*255.0);
					drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
					glColor4ub(255, 255, 255, 255);
				}
			}
			break;
		case STATE_SECRET:
			if(win == 2 && splashPassing == 0){
				splashPassing = 180;
				win = 20;
			}
			drawTexture(0, 0, 200, 150, win2);
			if(win == 2){
				glColor4ub(0, 0, 0, (float)(splashPassing)/180.0*255.0);
				drawRect(0, 0, 200, 150);
				glColor4ub(255, 255, 255, 255);
			}
			break;
		default:;
	}
	if(GAME_SHOWFPS) renderFramerate();
	deactivateFramebuffer();
	glScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	drawTexture(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, screenTex);
	
	SDL_GL_SwapBuffers();
}

void gameWin(int ending){
	splashPassing = 179;
	win = ending;
	if(ending == 2) state = STATE_SECRET;
}

void game(GLuint screenTex){
	float delayVal;
	
	removeMatchingFiles(PATH_SAVEBASE, PATH_LEVELSAVE_EXT);
	initGame();
	
	resetFrames();
	running = 1;
	while(running){
		handleInput();
		
		framesToRender = (float)SDL_GetTicks() / GAME_DELAY;
		if(framesToRender == framesRendered){
			delayVal = (float)(framesToRender+1) * GAME_DELAY - SDL_GetTicks();
			if (delayVal <= 0.0) continue;
			SDL_Delay(delayVal);
			continue;
		}
		if(framesToRender-framesRendered > GAME_MAXFRAMESKIP){
			framesRendered = framesToRender - GAME_MAXFRAMESKIP;
			warning("Framerate too low");
		}
		
		while(framesRendered < framesToRender){
			if(framesToRender-framesRendered > 1){
				// Partial frame
				update(1);
			} else {
				// Full frame
				update(0);
				redraw(screenTex);
			}
			
			++framesRendered;
		}
	}
	
	cleanupGame();
}

void resetFrames(void){
	framesToRender = framesRendered = (float)SDL_GetTicks() / GAME_DELAY;
}
