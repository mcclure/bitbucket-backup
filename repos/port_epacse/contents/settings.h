#ifndef SETTINGS_H
#define SETTINGS_H

#include <SDL/SDL.h>

// Video settings
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int SCREEN_ZOOM;
extern Uint32 SCREEN_FLAGS;
extern char* SCREEN_CAPTION;
extern float GAME_FPS;
extern float GAME_DELAY;
extern int GAME_SHOWFPS;
extern int GAME_MAXFRAMESKIP;

// Sound settings
extern float SOUND_PANNINGSCOPE;

// Keymappings
extern int KEY_LEFT;
extern int KEY_RIGHT;
extern int KEY_UP;
extern int KEY_CROUCH;
extern int KEY_QUIT;

// Crucial game settings
extern int DOTMATRIX_W;
extern int DOTMATRIX_H;
extern char* LEVELSAVE_HEADER;
extern int MOUSEBLAST_RADIUS;
extern int MOUSEBLAST_POWER;

// Element definitions
extern int DOT_SUBPIXELRES;
extern int DOT_MAXCHANCE;
extern int WATER_R;
extern int WATER_G;
extern int WATER_B;
extern int WATER_A;
extern int WATER_GRAVITY;
extern int WATER_MAXFALLSPD;
extern int WATER_XDECCELERATION;
extern int WATER_SIDECHANCE;
extern int WATER_SLIDEMULTIPLIER;
extern int WATER_WEIGHT;
extern float WATER_OPACITY;
extern int SAND_R;
extern int SAND_G;
extern int SAND_B;
extern int SAND_A;
extern int SAND_GRAVITY;
extern int SAND_MAXFALLSPD;
extern int SAND_XDECCELERATION;
extern int SAND_SIDECHANCE;
extern int SAND_SLIDEMULTIPLIER;
extern int SAND_WEIGHT;
extern float SAND_OPACITY;

// Player physics
extern int PLAYER_SUBPIXELRES; 
extern int PLAYER_WALKACC;
extern int PLAYER_JUMPSTRENGTH;
extern int PLAYER_JUMPFRAMES;
extern int PLAYER_GRAVITY;
extern int PLAYER_MAXWALKSPD;
extern int PLAYER_BRAKING;
extern int PLAYER_AIRBRAKING;
extern int PLAYER_MAXFALLSPD;
extern int PLAYER_WALKSPRITEIVL;
extern int PLAYER_CROUCHJUMP;

// Player environment modifiers
extern float UNDERWATER_JUMPSTRENGTH;
extern float UNDERWATER_CROUCHJUMP;
extern float UNDERWATER_MAXFALLSPD;
extern float UNDERWATER_MAXWALKSPD;
extern float UNDERWATER_WALKACC;
extern float UNDERWATER_BRAKING;

void loadSettings(const char* config);
void toggleFPS(void);

#endif // SETTINGS_H
