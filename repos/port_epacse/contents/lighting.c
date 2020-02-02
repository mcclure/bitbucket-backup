#include "lighting.h"

#include "error.h"
#include "graphics.h"
#include "settings.h"
#include "dotmatrix.h"
#include "player.h"

static int sourcesNum = 0;
static LightSource* sources = NULL;

static float* lightMatrix = NULL;
static float* lightMatrixMax = NULL;
static float* lightMatrixMin = NULL;

static void fillLightMatrix(float* matrix, float val){
	int i;
	for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
		matrix[i] = val;
	}
}

void initLighting(void){
	lightMatrix    = malloc(sizeof(float)*DOTMATRIX_W*DOTMATRIX_H);
	lightMatrixMax = malloc(sizeof(float)*DOTMATRIX_W*DOTMATRIX_H);
	lightMatrixMin = malloc(sizeof(float)*DOTMATRIX_W*DOTMATRIX_H);
}

void cleanupLighting(void){
	clearLightSources();
	free(lightMatrixMin);
	lightMatrixMin = NULL;
	free(lightMatrixMax);
	lightMatrixMax = NULL;
	free(lightMatrix);
	lightMatrix = NULL;
}

void allocateLightSources(int num){
	LightSource* oldsrc;
	
	if(num <= sourcesNum) return;
	oldsrc = sources;
	sources = realloc(sources, num*sizeof(LightSource));
	sourcesNum = num;
	if(sources == NULL){
		if(oldsrc != NULL) free(oldsrc);
		errorarg(ERROR_MEMALLOC, "sources");
	}
}

void clearLightSources(void){
	if(sources != NULL){
		free(sources);
		sources = NULL;
		sourcesNum = 0;
	}
}

LightSource* getLightSource(int num){
	return sources+num;
}

void calculateLighting(void){
	LightSource* source;
	int x, y, signX, signY, procX, procY, deltaX, deltaY;
	int err, err2;
	float current;
	int i, j, pos, calculating;
	float lightStep;
	Dot* dot;
	
	for(i = 0; i < sourcesNum; ++i){
		fillLightMatrix(lightMatrixMax, 255.0);
		source = sources+i;
		
		x = y = 0;
		switch(source->model){
			case LIGHT_NONE:
				return;
			case LIGHT_PLAYER:
				source->x = getPlayerEyeX();
				source->y = getPlayerEyeY();
				break;
			case LIGHT_POINT:
				break; // Huh
			case LIGHT_TOP:
				y = DOTMATRIX_H-1;
				if(source->angle < 0){
					source->x = source->angle;
					source->y = 0;
				} else {
					x = -source->angle;
					source->x = source->y = 0;
				}
				break;
		}
		
		calculating = 1;
		while(calculating){
			deltaX = abs(x - source->x);
			deltaY = abs(y - source->y);
			signX = (source->x < x) ? 1 : -1;
			signY = (source->y < y) ? 1 : -1;
			err = deltaX - deltaY;
			procX = source->x;
			procY = source->y;
			current = 0.0;
			
			while(1){
				if(current < source->power){
					dot = getDotExt(procX, procY);
					if(dot != NULL)
						current += dot->type->opacity;
				} else
					current = source->power;
				
				if(procX >= 0 && procX < DOTMATRIX_W && procY >= 0 && procY < DOTMATRIX_H){
					pos = procY*DOTMATRIX_W+procX;
					if(lightMatrixMax[pos] == 255){
						lightMatrixMax[pos] = lightMatrixMin[pos] = current;
					}
					else {
						if(current < lightMatrixMin[pos])
							lightMatrixMin[pos] = current; else
						if(current > lightMatrixMax[pos]) lightMatrixMax[pos] = current;
					}
				}
				
				if(procX == x && procY == y)
					break;
				
				err2 = 2 * err;
				if(err2 > -deltaY){
					err -= deltaY;
					procX += signX;
				}
				if(err2 < deltaX){
					err += deltaX;
					procY += signY;
				}
			}
			
			switch(source->model){
				case LIGHT_POINT:
				case LIGHT_PLAYER:
					if(x == 0 && y == 1) calculating = 0;
					if(x == 0 && y > 1) --y;
					if(x > 0 && y == DOTMATRIX_H-1) --x;
					if(x == DOTMATRIX_W-1 && y < DOTMATRIX_H-1) ++y;
					if(x < DOTMATRIX_W-1 && y == 0) ++x;
					break;
				case LIGHT_TOP:
					if(x >= DOTMATRIX_W-1 && source->x >= DOTMATRIX_W-1) calculating = 0;
					++x;
					++source->x;
					break;
				default:;
			}
		}
		
		lightStep = sources->shadow / source->power * source->brightness;
		for(j = 0; j < DOTMATRIX_W*DOTMATRIX_H; ++j){
			if(i == 0)
				lightMatrix[j] = (1.0-source->brightness) * source->shadow + (lightMatrixMax[j]+lightMatrixMin[j])/2 * lightStep;
			else
				lightMatrix[j] -= (source->power - (lightMatrixMax[j]+lightMatrixMin[j])/2) * lightStep;
			if(lightMatrix[j] < 0.0) lightMatrix[j] = 0.0;
		}
	}
}

void renderLighting(){
	int i;
	
	glBegin(GL_POINTS);
		for(i = 0; i < DOTMATRIX_W*DOTMATRIX_H; ++i){
			glColor4f(0, 0, 0, lightMatrix[i]);
			glVertex2i(getDotX(i), getDotY(i));
		}
	glEnd();
	glColor3ub(255, 255, 255);
}
