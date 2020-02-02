#include "settings.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#include "error.h"

int SCREEN_WIDTH;
int SCREEN_HEIGHT;
int SCREEN_ZOOM;
Uint32 SCREEN_FLAGS = SDL_OPENGL;
char* SCREEN_CAPTION = "epacse";
float GAME_FPS;
float GAME_DELAY;
int GAME_SHOWFPS = 0;
int GAME_MAXFRAMESKIP = 10;

float SOUND_PANNINGSCOPE;

int KEY_LEFT = SDLK_a;
int KEY_RIGHT = SDLK_d;
int KEY_UP = SDLK_w;
int KEY_CROUCH = SDLK_s;
int KEY_QUIT = SDLK_ESCAPE;

int DOTMATRIX_W = 200;
int DOTMATRIX_H = 150;
char* LEVELSAVE_HEADER = "EPACSEDOTSAVE1";
int MOUSEBLAST_RADIUS = 10;
int MOUSEBLAST_POWER = 75;

int DOT_SUBPIXELRES = 100;
int DOT_MAXCHANCE = 255;
int WATER_R = 31;
int WATER_G = 31;
int WATER_B = 255;
int WATER_A = 100;
int WATER_GRAVITY = 5;
int WATER_MAXFALLSPD = 300;
int WATER_XDECCELERATION = 1;
int WATER_SIDECHANCE = 10;
int WATER_SLIDEMULTIPLIER = 10;
int WATER_WEIGHT = 100;
float WATER_OPACITY = 0.1;
int SAND_R = 255;
int SAND_G = 211;
int SAND_B = 127;
int SAND_A = 255;
int SAND_GRAVITY = 5;
int SAND_MAXFALLSPD = 200;
int SAND_XDECCELERATION = 1;
int SAND_SIDECHANCE = 1;
int SAND_SLIDEMULTIPLIER = 0; // Will only scatter while falling
int SAND_WEIGHT = 110;
float SAND_OPACITY = 0.5;

int PLAYER_SUBPIXELRES = 100;
int PLAYER_WALKACC = 8;
int PLAYER_JUMPSTRENGTH = 175;
int PLAYER_JUMPFRAMES = 20;
int PLAYER_GRAVITY = 5;
int PLAYER_MAXWALKSPD = 75;
int PLAYER_BRAKING = 8;
int PLAYER_AIRBRAKING = 40;
int PLAYER_MAXFALLSPD = 300;
int PLAYER_WALKSPRITEIVL = 10;
int PLAYER_CROUCHJUMP = 105;

float UNDERWATER_JUMPSTRENGTH = 0.7;
float UNDERWATER_CROUCHJUMP = 0.7;
float UNDERWATER_MAXFALLSPD = 0.2;
float UNDERWATER_MAXWALKSPD = 0.6;
float UNDERWATER_WALKACC = 0.7;
float UNDERWATER_BRAKING = 0.6;

void loadSettings(const char* config){
	char line[256], name[64], value[16];
	FILE* file;
	
	file = fopen(config, "r");
	if(file == NULL) errorarg(ERROR_FILEREAD, config);
	while(1){
		if(fgets(line, 256, file) == NULL) break;
		if(strcmp(line, "") == 0 || strcmp(line, "\n") == 0) continue;
		if(line[0] == '#') continue;
		strcpy(name, "");
		strcpy(value, "0");
		sscanf(line, "%63s = %15s", name, value);
		     if(strcmp(name, "SCREEN_ZOOM") == 0) SCREEN_ZOOM = atoi(value);
		else if(strcmp(name, "GAME_FPS") == 0) GAME_FPS = atoi(value);
		else if(strcmp(name, "SOUND_PANNINGSCOPE") == 0) SOUND_PANNINGSCOPE = atof(value);
		else errorarg(ERROR_SYNTAX, config);
	}
	fclose(file);
	
	SCREEN_WIDTH = DOTMATRIX_W * SCREEN_ZOOM;
	SCREEN_HEIGHT = DOTMATRIX_H * SCREEN_ZOOM;
	GAME_DELAY = 1000.0/GAME_FPS;
}

void toggleFPS(void){
	if(GAME_SHOWFPS == 0) GAME_SHOWFPS = 1;
	else GAME_SHOWFPS = 0;
}
