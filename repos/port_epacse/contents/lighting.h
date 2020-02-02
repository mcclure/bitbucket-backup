#ifndef LIGHTING_H
#define LIGHTING_H

typedef enum LightModel {
	LIGHT_NONE = 0,
	LIGHT_POINT,
	LIGHT_PLAYER,
	LIGHT_TOP
} LightModel;

typedef struct LightSource {
	int x, y;
	int angle, power;
	float shadow, brightness;
	LightModel model;
} LightSource;

void initLighting(void);
void cleanupLighting(void);
void allocateLightSources(int num);
void clearLightSources(void);
LightSource* getLightSource(int num);
void calculateLighting(void);
void renderLighting(void);

#endif // LIGHTING_H
