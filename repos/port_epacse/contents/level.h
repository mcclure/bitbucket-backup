#ifndef LEVEL_H
#define LEVEL_H

#include "input.h"

void initLevels(const char* beginPath);
void cleanupLevels(void);

int getLevelX(void);
int getLevelY(void);
void changeLevel(int coordX, int coordY);
void changeLevelRel(int dx, int dy);
void updateLevel(Input* input, unsigned char partial);
void renderLevel();

#endif // LEVEL_H
