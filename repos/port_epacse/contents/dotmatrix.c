#include "dotmatrix.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <SDL/SDL_opengl.h>
#include <SOIL.h>
#include "settings.h"
#include "paths.h"
#include "error.h"
#include "graphics.h"
#include "utils.h"
#include "zpipe.h"
#include "dot.h"
#include "dotgen.h"
#include "lighting.h"
#include "player.h"

typedef struct DotMatrix {
	int x, y;
	Dot** dots;
	unsigned char active;
} DotMatrix;

static int matricesNum = 0;
static DotMatrix* matrices = NULL;
static int activeMatrix = -1;
static Dot** activeDots = NULL;

static inline int getDotIndex(int x, int y){
	return y*DOTMATRIX_W + x;
}

static unsigned char canMoveUp(int index){
	if(index - DOTMATRIX_W < 0) return 1;
	if(activeDots[index - DOTMATRIX_W] != NULL) return 0;
	return 1;
}

static unsigned char canMoveDown(int index){
	if(index + DOTMATRIX_W >= DOTMATRIX_W*DOTMATRIX_H) return 1;
	if(activeDots[index + DOTMATRIX_W] != NULL) return 0;
	return 1;
}

static unsigned char canMoveLeft(int index){
	if(index - 1 < 0) return 1;
	if(getDotY(index) != getDotY(index-1)) { if(matrices[activeMatrix].x == 8 && matrices[activeMatrix].y == 1) return 0; else return 1; }
	if(activeDots[index - 1] != NULL) return 0;
	return 1;
}

static unsigned char canMoveRight(int index){
	if(index + 1 >= DOTMATRIX_W*DOTMATRIX_H) return 1;
	if(getDotY(index) != getDotY(index+1)) { if(matrices[activeMatrix].x == 7 && matrices[activeMatrix].y == 1) return 0; else return 1; }
	if(activeDots[index + 1] != NULL) return 0;
	return 1;
}

static void moveDot(int from, int to){
	if(from < 0 || from >= DOTMATRIX_W*DOTMATRIX_H) return;
	if(to < 0 || to >= DOTMATRIX_W*DOTMATRIX_H || (getDotX(from) != getDotX(to) && getDotY(from) != getDotY(to))){
		if(activeDots[from] != NULL) free(activeDots[from]);
		activeDots[from] = NULL;
		return;
	}
	activeDots[to] = activeDots[from];
	activeDots[from] = NULL;
}

static void swapDots(int from, int to){
	Dot* temp;
	if(from < 0 || from >= DOTMATRIX_W*DOTMATRIX_H) return;
	if(to < 0 || to >= DOTMATRIX_W*DOTMATRIX_H || (getDotX(from) != getDotX(to) && getDotY(from) != getDotY(to))) return;
	temp = activeDots[to];
	activeDots[to] = activeDots[from];
	activeDots[from] = temp;
}

static void clearDotMatrices(void){
	int i, j;
	for(j = 0; j < matricesNum; ++j){
		if(matrices[j].dots == NULL) continue;
		Dot** iter = matrices[j].dots;
		for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i, ++iter){
			if(*iter != NULL){
				free(*iter);
				*iter = NULL;
			}
		}
		matrices[j].active = 0;
	}
}

void addDotMatrix(int x, int y){
	DotMatrix* prev;
	prev = matrices;
	matrices = realloc(matrices, (matricesNum+1)*sizeof(DotMatrix));
	if(matrices == NULL) { free(prev); errorarg(ERROR_MEMALLOC, "matrices"); }
	matrices[matricesNum].x = x;
	matrices[matricesNum].y = y;
	matrices[matricesNum].active = 0;
	matrices[matricesNum].dots = calloc(DOTMATRIX_W*DOTMATRIX_H, sizeof(Dot*));
	if(matrices[matricesNum].dots == NULL) errorarg(ERROR_MEMALLOC, "matrices[].dots");
	++matricesNum;
}

unsigned char dotMatrixExists(int x, int y){
	int i;
	if(matrices == NULL) return 0;
	for(i = 0; i < matricesNum; ++i)
		if(matrices[i].x == x && matrices[i].y == y) return 1;
	
	return 0;
}

void activateDotMatrix(int x, int y){
	int i, matrix = -1;
	if(matrices == NULL) return;
	for(i = 0; i < matricesNum; ++i)
		if(matrices[i].x == x && matrices[i].y == y) matrix = i;
	if(matrix == -1) return;
	if(matrices[matrix].dots == NULL){
		matrices[matrix].dots = calloc(DOTMATRIX_W*DOTMATRIX_H, sizeof(Dot*));
		if(matrices[matrix].dots == NULL) errorarg(ERROR_MEMALLOC, "matrices[].dots");
	}
	activeMatrix = matrix;
	activeDots = matrices[matrix].dots;
	matrices[matrix].active = 1;
}

void deactivateDotMatrix(int x, int y){
	int i;
	if(matrices == NULL) return;
	for(i = 0; i < matricesNum; ++i)
		if(matrices[i].x == x && matrices[i].y == y) { matrices[i].active = 0; return; }
}


void cleanInactiveMatrices(void){
	int i;
	for(i = 0; i < matricesNum; ++i){
		if(!matrices[i].active){
			free(matrices[i].dots);
			matrices[i].dots = NULL;
		}
	}
}

int getDotX(int index){
	return index%DOTMATRIX_W;
}

int getDotY(int index){
	return index/DOTMATRIX_W;
}

void initDotMatrix(void){
	initLighting();
}

void cleanupDotMatrix(void){
	int i;
	cleanupLighting();
	clearDotGenerators();
	clearDotMatrices();
	for(i = 0; i < matricesNum; ++i)
		if(matrices[i].dots != NULL) free(matrices[i].dots);
	free(matrices);
	matrices = NULL;
}

void clearDotMatrix(void){
	int i;
	Dot** iter = activeDots;
	for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i, ++iter){
		if(*iter != NULL){
			free(*iter);
			*iter = NULL;
		}
	}
}

static void addDotTyped(Dot* dot, int x, int y){
	if(activeDots[getDotIndex(x,y)] != NULL) { free(dot); return; }
	activeDots[getDotIndex(x,y)] = dot;
}

void loadDataFile(const char* data){
	FILE* file;
	char line[256], name[64], value[16];
	int source = 0, gen = 0;
	
	file = fopen(data, "r");
	if(file == NULL) errorarg(ERROR_FILEREAD, data);
	
	clearLightSources();
	clearDotGenerators();
	while(1){
		if(fgets(line, 256, file) == NULL) break;
		if(strcmp(line, "") == 0 || strcmp(line, "\n") == 0) continue;
		if(line[0] == '#') continue;
		strcpy(name, "");
		strcpy(value, "0");
		sscanf(line, "%63s = %15s", name, value);
			
		if(strcmp(name, "LIGHTSOURCE")                 == 0){
			source = atoi(value)-1;
			allocateLightSources(source+1);
		}
		else if(strcmp(name, "LIGHTSOURCE_MODEL")      == 0){
			     if(strcmp(value, "POINT")             == 0) getLightSource(source)->model = LIGHT_POINT;
			else if(strcmp(value, "PLAYER")            == 0) getLightSource(source)->model = LIGHT_PLAYER;
			else if(strcmp(value, "TOP")               == 0) getLightSource(source)->model = LIGHT_TOP;
			else                                             errorarg(ERROR_SYNTAX, data);
		}
		else if(strcmp(name, "LIGHTSOURCE_X")          == 0) { getLightSource(source)->x = atoi(value); getLightSource(source)->model = LIGHT_POINT; }
		else if(strcmp(name, "LIGHTSOURCE_Y")          == 0) { getLightSource(source)->y = atoi(value); getLightSource(source)->model = LIGHT_POINT; }
		else if(strcmp(name, "LIGHTSOURCE_ANGLE")      == 0) { getLightSource(source)->angle = atoi(value); getLightSource(source)->model = LIGHT_TOP; }
		else if(strcmp(name, "LIGHTSOURCE_POWER")      == 0) getLightSource(source)->power = atoi(value);
		else if(strcmp(name, "LIGHTSOURCE_SHADOW")     == 0) getLightSource(source)->shadow = atof(value);
		else if(strcmp(name, "LIGHTSOURCE_BRIGHTNESS") == 0) getLightSource(source)->brightness = atof(value);
		
		else if(strcmp(name, "DOTGENERATOR")           == 0){
			gen = atoi(value)-1;
			allocateDotGenerators(gen+1);
		}
		else if(strcmp(name, "DOTGENERATOR_X")         == 0) getDotGenerator(gen)->x = atoi(value);
		else if(strcmp(name, "DOTGENERATOR_Y")         == 0) getDotGenerator(gen)->y = atoi(value);
		else if(strcmp(name, "DOTGENERATOR_DELAY")     == 0) getDotGenerator(gen)->delayMin = getDotGenerator(gen)->delayMax = atoi(value);
		else if(strcmp(name, "DOTGENERATOR_DELAYMIN")  == 0) getDotGenerator(gen)->delayMin = atoi(value);
		else if(strcmp(name, "DOTGENERATOR_DELAYMAX")  == 0) getDotGenerator(gen)->delayMax = atoi(value);
		else if(strcmp(name, "DOTGENERATOR_DOTTYPE")   == 0) getDotGenerator(gen)->type = getDotTypeFromString(value);
		else if(strcmp(name, "DOTGENERATOR_DOTDX")     == 0) getDotGenerator(gen)->dotDx = atoi(value);
		else if(strcmp(name, "DOTGENERATOR_DOTDY")     == 0) getDotGenerator(gen)->dotDy = atoi(value);
		
		else errorarg(ERROR_SYNTAX, data);
	}
	fclose(file);
}

void loadDotMatrixFromImages(const char* level, const char* dynamic){
	Dot* dot;
	int i;
	Uint8 r, g, b, a;
	unsigned char* image;
	unsigned char* imagePtr;
	int width, height;
	
	image = SOIL_load_image(level, &width, &height, NULL, SOIL_LOAD_RGBA);
	if(image == 0) errorarg2(ERROR_IMGOPEN, level, SOIL_last_result());
	
	if(width != DOTMATRIX_W || height != DOTMATRIX_H) errorarg("Wrong level dimensions", level);
	imagePtr = image;
	for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
		r = *imagePtr; ++imagePtr;
		g = *imagePtr; ++imagePtr;
		b = *imagePtr; ++imagePtr;
		a = *imagePtr; ++imagePtr;
		if(a == 0) continue;
		dot = createDot();
		dot->r = r;
		dot->g = g;
		dot->b = b;
		dot->a = a;
		addDotStruct(dot, DOT_BLOCK);
		addDotTyped(dot, getDotX(i), getDotY(i));
	}
	
	SOIL_free_image_data(image);
	
	image = SOIL_load_image(dynamic, &width, &height, NULL, SOIL_LOAD_RGBA);
	if(image == 0) errorarg2(ERROR_IMGOPEN, dynamic, SOIL_last_result());
	
	if(width != DOTMATRIX_W || height != DOTMATRIX_H) errorarg("Wrong level dimensions", dynamic);
	imagePtr = image;
	for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
		r = *imagePtr; ++imagePtr;
		g = *imagePtr; ++imagePtr;
		b = *imagePtr; ++imagePtr;
		a = *imagePtr; ++imagePtr;
		if(a == 0) continue;
		dot = createDot();
			 if(r == WATER_R && g == WATER_G && b == WATER_B) addDotStruct(dot, DOT_WATER);
		else if(r == SAND_R  && g == SAND_G  && b == SAND_B ) addDotStruct(dot, DOT_SAND);
		else errorarg("Invalid color", dynamic);
		addDotTyped(dot, getDotX(i), getDotY(i));
	}
	
	SOIL_free_image_data(image);
}

void loadDotMatrixFromFile(const char* save){
	FILE* cmprssd;
	FILE* temp;
	char* headerBuf;
	Dot* newDot;
	dotType newType;
	int i;
	
	cmprssd = fopen(save, "rb");
	temp = fopen(PATH_TEMPORARY, "wb");
	i = inf(cmprssd, temp);
	if(i != Z_OK) errorarg("ZLib error", zerr(i));
	fclose(cmprssd);
	
	if(freopen(PATH_TEMPORARY, "rb", temp) == NULL) errorarg(ERROR_FILEREAD, save);
	headerBuf = malloc(strlen(LEVELSAVE_HEADER)+1);
	if(fread(headerBuf, sizeof(char), strlen(LEVELSAVE_HEADER), temp) != strlen(LEVELSAVE_HEADER))
		errorarg("Invalid save file", save);
	headerBuf[strlen(LEVELSAVE_HEADER)] = '\0';
	if(strcmp(headerBuf, LEVELSAVE_HEADER) != 0)
		errorarg("Invalid save file", save);
	
	clearDotMatrix();
	for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
		if(fread(&newType, sizeof(newType), 1, temp) != 1) errorarg("Save file corrupted", save);
		if(newType == DOT_NONE){
			fseek(temp, sizeof(int)*4 + sizeof(unsigned char)*4, SEEK_CUR); // Skipping the rest of the fields
		} else {
			newDot = malloc(sizeof(Dot));
			addDotStruct(newDot, newType);
			newDot->iteration = 0;
			if(fread(&newDot->mx, sizeof(int), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->my, sizeof(int), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->dx, sizeof(int), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->dy, sizeof(int), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->r, sizeof(unsigned char), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->g, sizeof(unsigned char), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->b, sizeof(unsigned char), 1, temp) != 1) errorarg("Save file corrupted", save);
			if(fread(&newDot->a, sizeof(unsigned char), 1, temp) != 1) errorarg("Save file corrupted", save);
			activeDots[i] = newDot;
		}
	}
	
	fclose(temp);
	free(headerBuf);
	remove(PATH_TEMPORARY);
}

void saveDotMatrixToFile(const char* save){
	FILE* temp;
	FILE* cmprssd; // Lol
	Dot empty;
	dotType emptyType = DOT_NONE;
	Dot* current;
	int i;
	
	empty.r = empty.g = empty.b = empty.a = empty.dx = empty.dy = empty.iteration = empty.mx = empty.my = 0;
	
	temp = fopen(PATH_TEMPORARY, "wb");
	if(temp == NULL) errorarg(ERROR_FILEWRITE, PATH_TEMPORARY);
	fprintf(temp, "%s", LEVELSAVE_HEADER);
	
	for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
		if(activeDots[i] == NULL){
			current = &empty;
			fwrite(&emptyType, sizeof(dotType), 1, temp);
		} else {
			current = activeDots[i];
			fwrite(&current->type->type, sizeof(dotType), 1, temp);
		}
		fwrite(&current->mx, sizeof(int), 1, temp);
		fwrite(&current->my, sizeof(int), 1, temp);
		fwrite(&current->dx, sizeof(int), 1, temp);
		fwrite(&current->dy, sizeof(int), 1, temp);
		fwrite(&current->r, sizeof(unsigned char), 1, temp);
		fwrite(&current->g, sizeof(unsigned char), 1, temp);
		fwrite(&current->b, sizeof(unsigned char), 1, temp);
		fwrite(&current->a, sizeof(unsigned char), 1, temp);
	}
	
	if(freopen(PATH_TEMPORARY, "rb", temp) == NULL) errorarg(ERROR_FILEREAD, PATH_TEMPORARY);
	cmprssd = fopen(save, "wb");
	i = def(temp, cmprssd, 1); // Woot variable reuse
	if(i != Z_OK) errorarg("ZLib error", zerr(i));
	fclose(cmprssd);
	fclose(temp);
	remove(PATH_TEMPORARY);
}

unsigned char dotMatrixSaveExists(const char* save){
	FILE* file;
	
	file = fopen(save, "rb");
	if(file == NULL)
		return 0;
	else {
		fclose(file);
		return 1;
	}
}

void addDot(Dot* dot, dotType type, int x, int y){
	if(activeDots[getDotIndex(x,y)] != NULL || type == DOT_NONE) { free(dot); return; }
	addDotStruct(dot, type);
	activeDots[getDotIndex(x,y)] = dot;
}

void deleteDot(int x, int y){
	int index = getDotIndex(x, y);
	if(activeDots[index] == NULL) return;
	free(activeDots[index]);
	activeDots[index] = NULL;
}

Dot* getDot(int x, int y){
	if(x < 0 || x >= DOTMATRIX_W || y < 0 || y >= DOTMATRIX_H) return NULL;
	return activeDots[getDotIndex(x, y)];
}

Dot* getDotExt(int x, int y){
	if(x < 0) x = 0;
	else if(x >= DOTMATRIX_W) x = DOTMATRIX_W-1;
	if(y < 0) y = 0;
	else if(y >= DOTMATRIX_H) y = DOTMATRIX_H-1;
	return activeDots[getDotIndex(x, y)];
}

dotType getDotTypePos(int x, int y){
	Dot* dot = getDot(x, y);
	if(dot == NULL) return DOT_NONE;
	return getDotType(dot);
}

void updateDotMatrix(void){
	int i, j, proc, prevActive;
	int bigContinue;
	static int expectedIter = 0;
	Dot** iter;
	Dot* dot;
	int remainingSpd, secondWeight = 0;
	int gravity, maxFallSpd, xDecceleration, sideChance, slideMultiplier, weight;
	
	prevActive = activeMatrix;
	for(j = 0; j < matricesNum; ++j){
		if(!matrices[j].active || matrices[j].dots == NULL) continue;
		iter = matrices[j].dots + DOTMATRIX_W*DOTMATRIX_H - 1;
		activeMatrix = j;
		activeDots = matrices[j].dots;
		
		for(i = DOTMATRIX_W*DOTMATRIX_H-1; i >= 0; --i, --iter){
			if(*iter == NULL) continue;
			dot = *iter;
			if(dot->iteration != expectedIter) continue;
			if(getDotType(dot) == DOT_BLOCK) continue;
			proc = i;
			bigContinue = 0;
			
			gravity = dot->type->gravity;
			maxFallSpd = dot->type->maxFallSpd;
			xDecceleration = dot->type->xDecceleration;
			sideChance = dot->type->sideChance;
			slideMultiplier = dot->type->slideMultiplier;
			weight = dot->type->weight;
			
			// Gravity
			dot->dy += gravity;
			if(dot->dy > maxFallSpd) dot->dy = maxFallSpd;
			
			// Horizontal decceleration
			if(dot->dx > 0){
				if(dot->dx >= xDecceleration){
					dot->dx -= xDecceleration;
				} else {
					dot->dx = 0;
				}
			} else
			if(dot->dx < 0){
				if(dot->dx <= -xDecceleration){
					dot->dx += xDecceleration;
				} else {
					dot->dx = 0;
				}
			}
			
			// Weight
			while(proc+DOTMATRIX_W <= DOTMATRIX_W*DOTMATRIX_H && // This never loops
				  activeDots[proc+DOTMATRIX_W] != NULL &&
				  getDotType(activeDots[proc+DOTMATRIX_W]) != DOT_BLOCK){
				secondWeight = activeDots[proc+DOTMATRIX_W]->type->weight;
				if(weight <= secondWeight) break;
				if(randint(DOT_MAXCHANCE) < weight-secondWeight){
					swapDots(proc, proc+DOTMATRIX_W);
					if(activeDots[proc] != dot){
						proc += DOTMATRIX_W;
						if(activeDots[proc] != dot) bigContinue = 1;
					}
				}
				break;
			}
			if(bigContinue) continue;
			
			// Moving down
			if(dot->dy > 0){
				remainingSpd = dot->dy;
				while(remainingSpd >= DOT_SUBPIXELRES){
					if(canMoveDown(proc)){
						moveDot(proc, proc+DOTMATRIX_W);
						proc += DOTMATRIX_W;
						if(proc >= DOTMATRIX_W*DOTMATRIX_H){
							bigContinue = 1;
							break;
						}
						if(activeDots[proc] != dot){
							bigContinue = 1;
							break;
						}
						remainingSpd -= DOT_SUBPIXELRES;
					} else {
						dot->my = DOT_SUBPIXELRES-1;
						dot->dy = 0;
						remainingSpd = 0;
						break;
					}
				}
				if(bigContinue) continue;
				if(dot->my + remainingSpd >= DOT_SUBPIXELRES){
					if(canMoveDown(proc)){
						moveDot(proc, proc+DOTMATRIX_W);
						proc += DOTMATRIX_W;
						if(proc > DOTMATRIX_W*DOTMATRIX_H) continue;
						if(activeDots[proc] != dot) continue;
						dot->my += remainingSpd - DOT_SUBPIXELRES;
					} else {
						dot->my = DOT_SUBPIXELRES-1;
						dot->dy = 0;
					}
				} else {
					dot->my += remainingSpd;
				}
			} else
			
			// Moving up
			if(dot->dy < 0){
				remainingSpd = -dot->dy;
				while(remainingSpd >= DOT_SUBPIXELRES){
					if(canMoveUp(proc)){
						moveDot(proc, proc-DOTMATRIX_W);
						proc -= DOTMATRIX_W;
						if(proc < 0){
							bigContinue = 1;
							break;
						}
						if(activeDots[proc] != dot){
							bigContinue = 1;
							break;
						}
						remainingSpd -= DOT_SUBPIXELRES;
					} else {
						dot->my = 0;
						dot->dy = 0;
						remainingSpd = 0;
						break;
					}
				}
				if(bigContinue) continue;
				if(dot->my - remainingSpd < 0){
					if(canMoveUp(proc)){
						moveDot(proc, proc-DOTMATRIX_W);
						proc -= DOTMATRIX_W;
						if(proc < 0) continue;
						if(activeDots[proc] != dot) continue;
						dot->my -= remainingSpd - DOT_SUBPIXELRES;
					} else {
						dot->my = 0;
						dot->dy = 0;
					}
				} else {
					dot->my -= remainingSpd;
				}
			}
			
			// Random sideways movement
			if(dot->dx == 0){
				if(!canMoveDown(proc)) sideChance *= slideMultiplier;
				if(randint(DOT_MAXCHANCE) < sideChance){
					remainingSpd = randint(2); // Saving memory reusing the variable. This looks ugly
					if(canMoveLeft(proc) || canMoveRight(proc)){
						if(!canMoveLeft(proc)) remainingSpd = 1;
						if(!canMoveRight(proc)) remainingSpd = 0;
						if(remainingSpd == 0){
							moveDot(proc, proc-1);
							--proc;
							if(activeDots[proc] != dot) continue;
						} else {
							moveDot(proc, proc+1);
							++proc;
							if(activeDots[proc] != dot) continue;
						}
					}
				}
			}
			
			// Moving left
			if(dot->dx < 0){
				remainingSpd = -dot->dx;
				while(remainingSpd >= DOT_SUBPIXELRES){
					if(canMoveLeft(proc)){
						moveDot(proc, proc-1);
						--proc;
						if(proc < 0){
							bigContinue = 1;
							break;
						}
						if(activeDots[proc] != dot){
							bigContinue = 1;
							break;
						}
						remainingSpd -= DOT_SUBPIXELRES;
					} else {
						dot->mx = 0;
						dot->dx = 0;
						remainingSpd = 0;
						break;
					}
				}
				if(bigContinue) continue;
				if(dot->mx - remainingSpd < 0){
					if(canMoveLeft(proc)){
						moveDot(proc, proc-1);
						--proc;
						if(proc < 0) continue;
						if(activeDots[proc] != dot) continue;
						dot->mx -= remainingSpd - DOT_SUBPIXELRES;
					} else {
						dot->mx = 0;
						dot->dx = 0;
					}
				} else {
					dot->mx -= remainingSpd;
				}
			} else
			
			// Moving right
			if(dot->dx > 0){
				remainingSpd = dot->dx;
				while(remainingSpd >= DOT_SUBPIXELRES){
					if(canMoveRight(proc)){
						moveDot(proc, proc+1);
						++proc;
						if(proc >= DOTMATRIX_W*DOTMATRIX_H){
							bigContinue = 1;
							break;
						}
						if(activeDots[proc] != dot){
							bigContinue = 1;
							break;
						}
						remainingSpd -= DOT_SUBPIXELRES;
					} else {
						dot->mx = DOT_SUBPIXELRES-1;
						dot->dx = 0;
						remainingSpd = 0;
						break;
					}
				}
				if(bigContinue) continue;
				if(dot->mx + remainingSpd >= DOT_SUBPIXELRES){
					if(canMoveRight(proc)){
						moveDot(proc, proc+1);
						++proc;
						if(proc > DOTMATRIX_W*DOTMATRIX_H) continue;
						if(activeDots[proc] != dot) continue;
						dot->mx += remainingSpd - DOT_SUBPIXELRES;
					} else {
						dot->mx = DOT_SUBPIXELRES-1;
						dot->dx = 0;
					}
				} else {
					dot->mx += remainingSpd;
				}
			}
			if(dot->iteration == 0) dot->iteration = 1;
			else dot->iteration = 0;
		}
	}
	activeMatrix = prevActive;
	activeDots = matrices[activeMatrix].dots;
	if(expectedIter == 0) expectedIter = 1;
	else expectedIter = 0;
	
	updateDotGenerators();
}

void renderDotMatrix(){
	int i;
	glBegin(GL_POINTS);
		for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
			if(activeDots[i] == NULL || getDotType(activeDots[i]) != DOT_BLOCK) continue;
			glColor4ub(activeDots[i]->r, activeDots[i]->g, activeDots[i]->b, activeDots[i]->a);
			glVertex2i(getDotX(i), getDotY(i));
		}
		for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
			if(activeDots[i] != NULL && getDotType(activeDots[i]) != DOT_BLOCK){
				glColor4ub(activeDots[i]->type->r, activeDots[i]->type->g, activeDots[i]->type->b, activeDots[i]->type->a);
				glVertex2i(getDotX(i), getDotY(i));
			}
		}
	glEnd();
	glColor3ub(255, 255 ,255);
}
