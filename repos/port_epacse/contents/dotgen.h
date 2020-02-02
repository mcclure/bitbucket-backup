#ifndef DOTGEN_H
#define DOTGEN_H

#include "dot.h"

typedef struct DotGenerator {
	int x, y;
	int delayMin, delayMax;
	int wait; // Internal implementation
	dotType type;
	int dotDx, dotDy;
} DotGenerator;

void allocateDotGenerators(int num);
void clearDotGenerators(void);
DotGenerator* getDotGenerator(int num);
void updateDotGenerators(void);

#endif // DOTGEN_H
