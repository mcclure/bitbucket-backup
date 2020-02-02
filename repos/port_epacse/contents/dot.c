#include "dot.h"

#include <stdlib.h>
#include <string.h>
#include "settings.h"
#include "error.h"

dotTypeStruct* blockType;
dotTypeStruct* waterType;
dotTypeStruct* sandType;

static dotTypeStruct* getDotStructFromType(dotType type){
	switch(type){
		case DOT_NONE:
			return NULL;
		case DOT_BLOCK:
			return blockType;
		case DOT_WATER:
			return waterType;
		case DOT_SAND:
			return sandType;
	}
	return NULL; // In Soviet Russia, you throw warnings at the compiler
}

void initDot(void){
	blockType = malloc(sizeof(dotTypeStruct));
	waterType = malloc(sizeof(dotTypeStruct));
	sandType = malloc(sizeof(dotTypeStruct));
	blockType->type = DOT_BLOCK;
	blockType->r = blockType->g = blockType->b = blockType->a = blockType->gravity = blockType->maxFallSpd = 0;
	blockType->xDecceleration =  blockType->sideChance = blockType->slideMultiplier  = blockType->weight = 0;
	blockType->opacity = 1.0;
	waterType->type = DOT_WATER;
	waterType->r = WATER_R;
	waterType->g = WATER_G;
	waterType->b = WATER_B;
	waterType->a = WATER_A;
	waterType->gravity = WATER_GRAVITY;
	waterType->maxFallSpd = WATER_MAXFALLSPD;
	waterType->xDecceleration = WATER_XDECCELERATION;
	waterType->sideChance = WATER_SIDECHANCE;
	waterType->slideMultiplier = WATER_SLIDEMULTIPLIER;
	waterType->weight = WATER_WEIGHT;
	waterType->opacity = WATER_OPACITY;
	sandType->type = DOT_SAND;
	sandType->r = SAND_R;
	sandType->g = SAND_G;
	sandType->b = SAND_B;
	sandType->a = SAND_A;
	sandType->gravity = SAND_GRAVITY;
	sandType->maxFallSpd = SAND_MAXFALLSPD;
	sandType->xDecceleration = SAND_XDECCELERATION;
	sandType->sideChance = SAND_SIDECHANCE;
	sandType->slideMultiplier = SAND_SLIDEMULTIPLIER;
	sandType->weight = SAND_WEIGHT;
	sandType->opacity = SAND_OPACITY;
}

void cleanupDot(void){
	free(sandType);
	free(waterType);
	free(blockType);
}

Dot* createDot(void){
	Dot* result = NULL;
	result = calloc(1,sizeof(Dot));
	if(result == NULL) errorarg(ERROR_MEMALLOC, "Dot");
	result->mx = result->my = DOT_SUBPIXELRES/2;
	return result;
}

dotType getDotType(Dot* dot){
	if(dot == NULL) return DOT_NONE;
	return dot->type->type;
}

void addDotStruct(Dot* dot, dotType type){
	dot->type = getDotStructFromType(type);
}

dotType getDotTypeFromString(const char* str){
	     if(strcmp(str, "DOT_BLOCK") == 0) return DOT_BLOCK;
	else if(strcmp(str, "DOT_WATER") == 0) return DOT_WATER;
	else if(strcmp(str, "DOT_SAND")  == 0) return DOT_SAND;
	return DOT_NONE;
}

unsigned char canMoveThroughDot(Dot* dot){
	if(dot == NULL) return 1;
	switch(getDotType(dot)){
		case DOT_BLOCK: return 0; break;
		case DOT_WATER: return 1; break;
		case DOT_SAND:  return 0; break;
		default: return 1;
	}
}
