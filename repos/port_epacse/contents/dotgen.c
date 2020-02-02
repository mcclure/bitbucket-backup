#include "dotgen.h"

#include <stdlib.h>
#include "error.h"
#include "utils.h"
#include "dot.h"
#include "dotmatrix.h"

#include <stdio.h>
#include "settings.h"

int genNum = 0;
DotGenerator* gens = NULL;

static void initWait(void){
	int i;
	for(i = 0; i < genNum; ++i){
		gens[i].wait = 0;
	}
}

void allocateDotGenerators(int num){
	DotGenerator* oldgen;
	
	if(num <= genNum) return;
	oldgen = gens;
	gens = realloc(gens, num*sizeof(DotGenerator));
	genNum = num;
	if(gens == NULL){
		if(oldgen != NULL) free(oldgen);
		errorarg(ERROR_MEMALLOC, "gens");
	}
	initWait();
}

DotGenerator* getDotGenerator(int num){
	return gens+num;
}

void clearDotGenerators(void){
	if(gens != NULL){
		free(gens);
		gens = NULL;
		genNum = 0;
	}
}

void updateDotGenerators(void){
	int i;
	DotGenerator* gen;
	Dot* newDot;
	
	for(i = 0; i < genNum; ++i){
		gen = gens+i;
		if(gen->wait != 0){
			--gen->wait;
			continue;
		}
		if(gen->delayMin == gen->delayMax)
			gen->wait = gen->delayMin - 1;
		else
			gen->wait = randint(gen->delayMax - gen->delayMin + 1) + gen->delayMin - 1;
		newDot = createDot();
		newDot->dx = gen->dotDx;
		newDot->dy = gen->dotDy;
		addDot(newDot, gen->type, gen->x, gen->y);
	}
}
