#ifndef _JUMPCORE_DISPLAY
#define _JUMPCORE_DISPLAY

#include <vector>

// Note: DRAW_DEBUG will likely break on iPhone or other ES platforms. And I'm not sure about GL2 mode.
// I'm not fixing this, it's for debugging anyway?
#define DRAW_DEBUG 0

void text_init();
void drawText(string text, double x, double y, double rot = 0, bool xcenter = true, bool ycenter = true);

// Vertexes, then color, then texture
#define VERT_STRIDE 2
#define COLOR_STRIDE 1
#define TEX_STRIDE 2
#define SIZE_STRIDE 1

// Util

void drawObject(void *ptr, void *data); // For chipmunk
cpVect screenToGL(int screenx, int screeny, double desiredZ);

static unsigned int packColor(float r, float g, float b, float a = 1) {
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

struct quadpile : public vector<float> {
	static vector<unsigned short> megaIndex;
	static void megaIndexEnsure(int count);
	void push(float x1, float y1, float x2, float y2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0, float *texes = NULL);
	void push(float x1, float y1, float x2, float y2, bool usecolor, float r, float g, float b, float a, subtexture_slice *s);
};

struct linepile : public vector<float> {
	void push(float x1, float y1, float x2, float y2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0);
	void push(cpVect v1, cpVect v2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0);
};

struct pile : public vector<float> {
    void push(cpVect vertex, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0, float pointSize = 0, float *texes = NULL);
};

#endif /* _JUMPCORE_DISPLAY */