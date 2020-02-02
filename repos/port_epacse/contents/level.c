#include "level.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h> // :o filename looks serious
#include <errno.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "settings.h"
#include "paths.h"
#include "error.h"
#include "utils.h"
#include "graphics.h"
#include "game.h"
#include "input.h"
#include "lighting.h"
#include "dotmatrix.h"
#include "player.h"

typedef struct Level {
	int x, y;
	GLuint background, foreground;
} Level;

Level* currentLevel = NULL;

static void findLevels(int x, int y){
	char* path;
	struct stat info;
	
	if(x < -99 || x > 999 || y < -99 || y > 999) return;
	if(dotMatrixExists(x, y)) return;
	path = malloc(strlen(PATH_LEVELDIR_TEMPLATE)+1);
	sprintf(path, PATH_LEVELDIR, x, y);
	if(stat(path, &info) == -1) { free(path); return; }
	if(!S_ISDIR(info.st_mode)) {free(path); return; }
	free(path);
	addDotMatrix(x, y);
	findLevels(x-1, y); // omg recursion!
	findLevels(x+1, y);
	findLevels(x, y-1);
	findLevels(x, y+1);
}

void initLevels(const char* beginPath){
	FILE* file;
	char line[256], name[64], value[16];
	int playerX = 0, playerY = 0, levelX = 0, levelY = 0;
	
	file = fopen(beginPath, "r");
	if(file == NULL) errorarg(ERROR_FILEREAD, beginPath);
	while(1){
		if(fgets(line, 256, file) == NULL) break;
		if(strcmp(line, "") == 0 || strcmp(line, "\n") == 0) continue;
		if(line[0] == '#') continue;
		strcpy(name, "");
		strcpy(value, "0");
		if(sscanf(line, "%63s = %15s", name, value) == EOF) break;
		     if(strcmp(name, "PLAYER_X") == 0) playerX = atoi(value);
		else if(strcmp(name, "PLAYER_Y") == 0) playerY = atoi(value);
		else if(strcmp(name, "LEVEL_X")  == 0) levelX = atoi(value);
		else if(strcmp(name, "LEVEL_Y")  == 0) levelY = atoi(value);
		else errorarg(ERROR_SYNTAX, beginPath);
	}
	fclose(file);
	
	currentLevel = calloc(1,sizeof(Level));
	if(currentLevel == NULL) errorarg(ERROR_MEMALLOC, "Level");
	
	initDotMatrix();
	initPlayer(PATH_PLAYERSPRITE);
	warpPlayer(playerX, playerY);
	currentLevel->x = currentLevel->y = 1000;
	findLevels(levelX, levelY);
	changeLevel(levelX, levelY);
}

void cleanupLevels(void){
	cleanupPlayer();
	cleanupDotMatrix();
	if(currentLevel->background != 0) deleteTexture(currentLevel->background);
	if(currentLevel->foreground != 0) deleteTexture(currentLevel->foreground);
	free(currentLevel);
}

int getLevelX(void){
	return currentLevel->x;
}

int getLevelY(void){
	return currentLevel->y;
}

void changeLevel(int coordX, int coordY){
	char* save;
	char* matrix;
	char* dynamic;
	char* background;
	char* foreground;
	char* data;
	
	save = malloc(strlen(PATH_LEVELSAVE_TEMPLATE)+1);
	if(currentLevel->x != 1000 && currentLevel->y != 1000){
		sprintf(save, PATH_LEVELSAVE, currentLevel->x, currentLevel->y);
		saveDotMatrixToFile(save);
		deactivateDotMatrix(currentLevel->x, currentLevel->y);
	}
	
	if(coordX < -98 || coordX > 998 || coordY < -98 || coordY > 998) error("Level index out of bounds");
	
	currentLevel->x = coordX;
	currentLevel->y = coordY;
	activateDotMatrix(coordX, coordY);
	cleanInactiveMatrices();
	
	sprintf(save, PATH_LEVELSAVE, coordX, coordY);
	data = malloc(strlen(PATH_LEVELDATA_TEMPLATE)+1);
	sprintf(data, PATH_LEVELDATA, coordX, coordY);
	if(fileExists(save)){
		loadDotMatrixFromFile(save);
		loadDataFile(data);
	} else {
		matrix = malloc(strlen(PATH_LEVELSTATIC_TEMPLATE)+1);
		dynamic = malloc(strlen(PATH_LEVELDYNAMIC_TEMPLATE)+1);
		sprintf(matrix, PATH_LEVELSTATIC, coordX, coordY);
		sprintf(dynamic, PATH_LEVELDYNAMIC, coordX, coordY);
		loadDotMatrixFromImages(matrix, dynamic);
		loadDataFile(data);
		free(dynamic);
		free(matrix);
	}
	
	background = malloc(strlen(PATH_LEVELBACKGROUND_TEMPLATE)+1);
	foreground = malloc(strlen(PATH_LEVELFOREGROUND_TEMPLATE)+1);
	sprintf(background, PATH_LEVELBACKGROUND, coordX, coordY);
	sprintf(foreground, PATH_LEVELFOREGROUND, coordX, coordY);
	
	if(currentLevel->background == 0) currentLevel->background = createTextureFromImage(background);
	else loadImageToTexture(background, currentLevel->background);
	
	if(currentLevel->foreground == 0) currentLevel->foreground = createTextureFromImage(foreground);
	else loadImageToTexture(foreground, currentLevel->foreground);
	
	free(data); // I just like freeing stuff in the reverse order of allocating.
	free(foreground);
	free(background);
	free(save);
	
	resetFrames();
}

void changeLevelRel(int dx, int dy){
	changeLevel(currentLevel->x+dx, currentLevel->y+dy);
}

void updateLevel(Input* input, unsigned char partial){
	// Mouseblast effect
	int x, y, ix, iy;
	float distance, direction;
	Dot* dot;
	if(SDL_GetMouseState(&x, &y) & SDL_BUTTON(1)){
		x /= SCREEN_ZOOM;
		y /= SCREEN_ZOOM;
		for(ix = x-MOUSEBLAST_RADIUS; ix <= x+MOUSEBLAST_RADIUS; ++ix)
		for(iy = y-MOUSEBLAST_RADIUS; iy <= y+MOUSEBLAST_RADIUS; ++iy){
			distance = sqrt((ix-x)*(ix-x) + (iy-y)*(iy-y));
			if(distance == 0.0 || distance > MOUSEBLAST_RADIUS) continue;
			dot = getDot(ix, iy);
			if(dot == NULL) continue;
			direction = atan2(iy-y, ix-x);
			dot->dx += cos(direction) * (MOUSEBLAST_RADIUS+1 - distance) * ((float)MOUSEBLAST_POWER/MOUSEBLAST_RADIUS);
			dot->dy += sin(direction) * (MOUSEBLAST_RADIUS+1 - distance) * ((float)MOUSEBLAST_POWER/MOUSEBLAST_RADIUS);
		}
	}
	updateDotMatrix();
	updatePlayer(input);
	if(!partial) calculateLighting();
}

void renderLevel(SDL_Surface* surface, int zoom){
	drawTexture(0, 0, SCREEN_WIDTH/SCREEN_ZOOM, SCREEN_HEIGHT/SCREEN_ZOOM, currentLevel->background);
	renderPlayer();
	renderDotMatrix();
	drawTexture(0, 0, SCREEN_WIDTH/SCREEN_ZOOM, SCREEN_HEIGHT/SCREEN_ZOOM, currentLevel->foreground);
	renderLighting();
}
