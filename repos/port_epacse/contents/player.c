#include "player.h"

#include <stdlib.h>
#include <SDL/SDL_opengl.h>
#include "settings.h"
#include "error.h"
#include "graphics.h"
#include "sound.h"
#include "input.h"
#include "level.h"
#include "dotmatrix.h"
#include "game.h"

typedef struct Player {
	int x, y;
	int dx, dy;
	unsigned w, h;
	unsigned jumpFrames;
	unsigned walkSprite;
	unsigned char crouching;
	
	GLuint image;
	unsigned sprite;
	unsigned direction;
} Player;

static Player* player = NULL;

#define PLAYER_X() (player->x/PLAYER_SUBPIXELRES)
#define PLAYER_Y() (player->y/PLAYER_SUBPIXELRES)

static inline unsigned char canMoveUp(void){
	int i;
	for(i = 0; i < player->w-2; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+1+i, PLAYER_Y()))) return 0;
	return 1;
}

static inline unsigned char canMoveDown(void){
	int i;
	for(i = 0; i < player->w-2; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+1+i, PLAYER_Y()+player->h))) return 0;
	return 1;
}

static inline unsigned char canMoveLeft(void){
	int i;
	for(i = 0; i < player->h-1; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X(), PLAYER_Y()+1+i))) return 0;
	return 1;
}

static inline unsigned char canMoveRight(void){
	int i;
	for(i = 0; i < player->h-1; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+player->w-1, PLAYER_Y()+1+i))) return 0;
	return 1;
}

static inline unsigned char canClimbUpLeft(void){
	int i;
	for(i = 0; i < player->h-1; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X(), PLAYER_Y()+i))) return 0;
	for(i = 0; i < player->w-2; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+i, PLAYER_Y()))) return 0;
	return 1;
}

static inline unsigned char canClimbUpRight(void){
	int i;
	for(i = 0; i < player->h-1; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+player->w-1, PLAYER_Y()+i))) return 0;
	for(i = 0; i < player->w-2; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+2+i, PLAYER_Y()))) return 0;
	return 1;
}

static inline unsigned char canClimbDownLeft(void){
	int i;
	for(i = 0; i < player->h-1; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X(), PLAYER_Y()+i))) return 0;
	for(i = 0; i < player->w-2; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+i, PLAYER_Y()+player->h))) return 0;
	return 1;
}

static inline unsigned char canClimbDownRight(void){
	int i;
	for(i = 0; i < player->h-1; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+player->w-1, PLAYER_Y()+i))) return 0;
	for(i = 0; i < player->w-2; ++i)
		if(!canMoveThroughDot(getDotExt(PLAYER_X()+2+i, PLAYER_Y()+player->h))) return 0;
	return 1;
}

static unsigned char isUnderwater(void){
	int x, y;
	int counter = 0;
	for(x = 1; x < player->w-1; ++x)
	for(y = 1 + player->crouching*2; y < player->h; ++y){
		if(getDotExt(PLAYER_X()+x, PLAYER_Y()+y) != NULL)
		if(getDotTypePos(PLAYER_X()+x, PLAYER_Y()+y) == DOT_WATER)
			++counter;
	}
	if(counter >= (player->w-2) * (player->h-1-player->crouching*2)/2) return 1;
	return 0;
}

static float playerPanning(void){
	return (float)(PLAYER_X()+2) / (DOTMATRIX_W-1) * 2.0 - 1.0;
}

void initPlayer(const char* path){
	player = calloc(1,sizeof(Player));
	if(player == NULL) errorarg(ERROR_MEMALLOC, "player");
	
	player->image = createTextureFromImage(path);
	player->x = player->y = PLAYER_SUBPIXELRES/2;
	player->w = 5;
	player->h = 10;
	player->walkSprite = PLAYER_WALKSPRITEIVL*4;
	player->direction = 1;
}

void cleanupPlayer(void){
	deleteTexture(player->image);
	free(player);
}

void warpPlayer(unsigned x, unsigned y){
	player->x = (x - player->w/2) * PLAYER_SUBPIXELRES + PLAYER_SUBPIXELRES/2;
	player->y = (y - player->h + 1) * PLAYER_SUBPIXELRES + PLAYER_SUBPIXELRES/2;
}

int getPlayerEyeX(void){
	int result = PLAYER_X();
	if(player->direction == 0) result += 1;
	else result += 3;
	return result;
}

int getPlayerEyeY(void){
	int result = PLAYER_Y();
	switch(player->sprite){
		case 5: result += 2; break;
		case 6: result += 4; break;
		default: result += 1;
	}
	return result;
}

void updatePlayer(Input* input){
	int jumpStrength, crouchJump, jumpFrames, walkAcc, braking, gravity, maxWalkSpd, maxFallSpd, airBraking;
	int pixelsToMove;

	// Initializing variables
	jumpStrength = PLAYER_JUMPSTRENGTH;
	crouchJump = PLAYER_CROUCHJUMP;
	jumpFrames = PLAYER_JUMPFRAMES;
	walkAcc = PLAYER_WALKACC;
	braking = PLAYER_BRAKING;
	gravity = PLAYER_GRAVITY;
	maxWalkSpd = PLAYER_MAXWALKSPD;
	maxFallSpd = PLAYER_MAXFALLSPD;
	airBraking = PLAYER_AIRBRAKING;
	
	// Underwater modifiers
	if(isUnderwater()){
		if(player->dy > maxFallSpd*UNDERWATER_MAXFALLSPD || abs(player->dx) > maxWalkSpd*UNDERWATER_MAXWALKSPD)
			playSoundPan(SOUND_SPLASH, playerPanning());
		jumpStrength *= UNDERWATER_JUMPSTRENGTH;
		crouchJump   *= UNDERWATER_CROUCHJUMP;
		maxFallSpd   *= UNDERWATER_MAXFALLSPD;
		maxWalkSpd   *= UNDERWATER_MAXWALKSPD;
		walkAcc      *= UNDERWATER_WALKACC;
		braking      *= UNDERWATER_BRAKING;
	}
	
	// Crouching
	if(input->crouch && !canMoveDown() && player->dx == 0) player->crouching = 1;
	else player->crouching = 0;
	
	// Jumping
	if(input->up){
		if(!canMoveDown() && player->dy >= 0){
			if(player->crouching){
				player->jumpFrames = jumpFrames;
				player->dy = -crouchJump;
				playSoundPan(SOUND_JUMP, playerPanning());
			} else {
				player->jumpFrames = jumpFrames;
				player->dy = -jumpStrength;
				playSoundPan(SOUND_JUMP, playerPanning());
			}
		}
	}
	if(player->dy > 0) player->jumpFrames = 1;
	if(player->jumpFrames > 0){
		--player->jumpFrames;
		if(player->jumpFrames == 0) input->up = 0;
	}
	
	// Walking
	if(input->left && !player->crouching)
		player->dx -= walkAcc;
	if(input->right && !player->crouching)
		player->dx += walkAcc;
	if((!input->left && !input->right) || (input->left && input->right)){
		if(player->dx > 0){
			if(player->dx > braking)
				player->dx -= braking;
			else 
				player->dx = 0;
		}
		else if(player->dx < 0){
			if(player->dx < -braking)
				player->dx += braking;
			else
				player->dx = 0;
		}
	}
	if(player->dx > maxWalkSpd) player->dx = maxWalkSpd; else
	if(player->dx < -maxWalkSpd) player->dx = -maxWalkSpd;
	
	// Gravity
	if(canMoveDown() || player->dy < 0) player->dy += gravity;
	if(player->dy > maxFallSpd) player->dy = maxFallSpd;
	if(player->jumpFrames > 0 && player->dy < 0 && !input->up) player->dy += maxFallSpd/airBraking;
	
	// Moving up
	if(player->dy < 0){
		pixelsToMove = PLAYER_Y() - (player->y + player->dy)/PLAYER_SUBPIXELRES;
		player->y = player->y + player->dy + pixelsToMove*PLAYER_SUBPIXELRES;
		while(pixelsToMove > 0){
			if(canMoveUp()){
				player->y -= PLAYER_SUBPIXELRES;
			} else {
				playSoundPan(SOUND_BUMP, playerPanning());
				input->up = 0;
				player->y = PLAYER_Y()*PLAYER_SUBPIXELRES;
				player->dy = 0;
				break;
			}
			--pixelsToMove;
		}
	} else
	
	// Moving down
	if(player->dy > 0){
		pixelsToMove = (player->y + player->dy)/PLAYER_SUBPIXELRES - PLAYER_Y();
		player->y = player->y + player->dy - pixelsToMove*PLAYER_SUBPIXELRES;
		while(pixelsToMove > 0){
			if(canMoveDown()){
				player->y += PLAYER_SUBPIXELRES;
			} else {
				playSoundPan(SOUND_BUMP, playerPanning());
				player->y = PLAYER_Y()*PLAYER_SUBPIXELRES + PLAYER_SUBPIXELRES-1;
				player->dy = 0;
				break;
			}
			--pixelsToMove;
		}
	}
	
	// Moving left
	if(player->dx < 0){
		pixelsToMove = PLAYER_X() - (player->x + player->dx)/PLAYER_SUBPIXELRES;
		player->x = player->x + player->dx + pixelsToMove*PLAYER_SUBPIXELRES;
		while(pixelsToMove > 0){
			if(canClimbDownLeft() && !canMoveDown()){
				player->x -= PLAYER_SUBPIXELRES;
				player->y += PLAYER_SUBPIXELRES;
				if(canMoveDown()) player->y -= PLAYER_SUBPIXELRES;
				--pixelsToMove;
				continue;
			}
			if(canMoveLeft()){
				player->x -= PLAYER_SUBPIXELRES;
			} else {
				if(canClimbUpLeft()){
					player->x -= PLAYER_SUBPIXELRES;
					player->y -= PLAYER_SUBPIXELRES;
				} else {
					player->dx = 0;
					player->x = PLAYER_X()*PLAYER_SUBPIXELRES;
					break;
				}
			}
			pixelsToMove--;
		}
	} else
	
	// Moving right
	if(player->dx > 0){
		pixelsToMove = (player->x + player->dx)/PLAYER_SUBPIXELRES - PLAYER_X();
		player->x = player->x + player->dx - pixelsToMove*PLAYER_SUBPIXELRES;
		while(pixelsToMove > 0){
			if(canClimbDownRight() && !canMoveDown()){
				player->x += PLAYER_SUBPIXELRES;
				player->y += PLAYER_SUBPIXELRES;
				if(canMoveDown()) player->y -= PLAYER_SUBPIXELRES;
				--pixelsToMove;
				continue;
			}
			if(canMoveRight()){
				player->x += PLAYER_SUBPIXELRES;
			} else {
				if(canClimbUpRight()){
					player->x += PLAYER_SUBPIXELRES;
					player->y -= PLAYER_SUBPIXELRES;
				} else {
					player->dx = 0;
					player->x = PLAYER_X()*PLAYER_SUBPIXELRES + PLAYER_SUBPIXELRES-1;
					break;
				}
			}
			--pixelsToMove;
		}
	}
	// Changing the level
		 if(PLAYER_X() < -2)            { if(getLevelX() == 7 && getLevelY() == 0) { gameWin(2); return; } player->x += (DOTMATRIX_W+1)*PLAYER_SUBPIXELRES; changeLevelRel(-1, 0); }
	else if(PLAYER_X() > DOTMATRIX_W-3) { player->x -= (DOTMATRIX_W+1)*PLAYER_SUBPIXELRES; changeLevelRel(+1, 0); }
	else if(PLAYER_Y() < -2)            { player->y += (DOTMATRIX_H-6)*PLAYER_SUBPIXELRES; changeLevelRel(0, -1); }
	else if(PLAYER_Y() > DOTMATRIX_H-9) { player->y -= (DOTMATRIX_H-5)*PLAYER_SUBPIXELRES; changeLevelRel(0, +1); }
	
	// Win condition
	if(getLevelX() == 9 && getLevelY() == 3 && PLAYER_Y() >= 14) { changeLevelRel(1, 0); gameWin(1); }
	
	// Setting the sprite
	if(player->dx < 0) player->direction = 0;
	if(player->dx > 0) player->direction = 1;
	if(player->dx == 0) player->sprite = 0;
	if(!canMoveDown() && player->dx != 0){
		if(input->left || input->right){
			if(player->walkSprite >= 3*PLAYER_WALKSPRITEIVL) player->sprite = 1;
			else if(player->walkSprite >= 2*PLAYER_WALKSPRITEIVL) player->sprite = 2;
			else if(player->walkSprite >= PLAYER_WALKSPRITEIVL) player->sprite = 3;
			else player->sprite = 2;
			if((player->dx < 0 && input->right) || (player->dx > 0 && input->left)) player->sprite = 0;
		} else {
			player->sprite = 0;
		}
	}
	if(player->dy < 0) player->sprite = 4;
	if(canMoveDown() && player->dy > 0) player->sprite = 5;
	--player->walkSprite;
	if(player->walkSprite == 0) player->walkSprite = 4*PLAYER_WALKSPRITEIVL;
	if(player->crouching) player->sprite = 6;
}

void renderPlayer(void){
	glBindTexture(GL_TEXTURE_2D, player->image);
	glBegin(GL_QUADS);
		glTexCoord2f((float)(player->sprite*5)/40, (float)(player->direction*10)/20);
		glVertex2i(PLAYER_X(), PLAYER_Y());
		glTexCoord2f((float)(player->sprite*5+5)/40, (float)(player->direction*10)/20);
		glVertex2i(PLAYER_X()+5, PLAYER_Y());
		glTexCoord2f((float)(player->sprite*5+5)/40, (float)(player->direction*10+10)/20);
		glVertex2i(PLAYER_X()+5, PLAYER_Y()+10);
		glTexCoord2f((float)(player->sprite*5)/40, (float)(player->direction*10+10)/20);
		glVertex2i(PLAYER_X(), PLAYER_Y()+10);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}
