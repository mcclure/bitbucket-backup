#ifndef PLAYER_H
#define PLAYER_H

#include "input.h"

void initPlayer(const char* path);
void cleanupPlayer(void);

void warpPlayer(unsigned x, unsigned y);
int getPlayerEyeX(void);
int getPlayerEyeY(void);

void updatePlayer(Input* input);
void renderPlayer(void);

#endif // PLAYER_H
