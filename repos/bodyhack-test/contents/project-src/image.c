#include "project.h"
#include "string.h"

// ASSUMES 32-BIT
EXPORT void set_color(SDL_Surface *surface, int x, int y, uint32_t color) {
	while (x<0) x +=surface->w;
	while (y<0) y +=surface->h;
	while (x>=surface->w) x -=surface->w;
	while (y>=surface->h) y -=surface->h;
	((uint32_t *)surface->pixels)[y*surface->w+x] = color;
}

EXPORT uint32_t get_color(SDL_Surface *surface, int x, int y) {
	while (x<0) x +=surface->w;
	while (y<0) y +=surface->h;
	while (x>=surface->w) x -=surface->w;
	while (y>=surface->h) y -=surface->h;
	return ((uint32_t *)surface->pixels)[y*surface->w+x];
}

EXPORT uint32_t get_component(uint32_t col, int i)
{
	return (col >> (i*8)) & 0xff;
}

// NO SAFETY ON THESE TWO
EXPORT void set_color_x(SDL_Surface *surface, int channel, int x, int y, uint32_t color) {
	uint32_t shifts[3] = {surface->format->Rshift, surface->format->Gshift, surface->format->Bshift};
	uint32_t masks[3] = {surface->format->Rmask, surface->format->Gmask, surface->format->Bmask};
	uint32_t mask = masks[channel];
	uint32_t shift = shifts[channel];
	uint32_t *pixels = surface->pixels;
	int idx = y*surface->w+x;
	pixels[idx] = (pixels[idx] & ~mask) | (color << shift);
}

EXPORT uint32_t get_color_x(SDL_Surface *surface, int channel, int x, int y) {
	uint32_t shifts[3] = {surface->format->Rshift, surface->format->Gshift, surface->format->Bshift};
	
	return (((uint32_t *)surface->pixels)[y*surface->w+x] >> shifts[channel]) & 0xFF;
}

#define RM 0xFF
#define GM 0xFF00
#define BM 0xFF0000
EXPORT uint32_t lesser(uint32_t c1, uint32_t c2) {
	uint32_t r1 = c1&RM, g1=c1&GM, b1=c1&BM;
	uint32_t r2 = c2&RM, g2=c2&GM, b2=c2&BM;
	
	return (r1>r2?r2:r1)|(g1>g2?g2:g1)|(b1>b2?b2:b1);
}

EXPORT uint32_t greater(uint32_t c1, uint32_t c2) {
	uint32_t r1 = c1&RM, g1=c1&GM, b1=c1&BM;
	uint32_t r2 = c2&RM, g2=c2&GM, b2=c2&BM;
	
	return (r1>r2?r1:r2)|(g1>g2?g1:g2)|(b1>b2?b1:b2);
}

/* Applies to load_image only.

Copyright (c) 2011 by Armin Ronacher.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//EXPORT void *load_image(SDL_Surface *out, const char *filename)
//{
//
//}

#if 0 // SDL_BYTEORDER == SDL_BIG_ENDIAN
const static int rmask = 0xff000000;
const static int gmask = 0x00ff0000;
const static int bmask = 0x0000ff00;
const static int amask = 0x000000ff;
#else
const static int rmask = 0x000000ff;
const static int gmask = 0x0000ff00;
const static int bmask = 0x00ff0000;
const static int amask = 0xff000000;
#endif

EXPORT SDL_Surface *load_image(const char *filename)
{
    int x, y, comp;
    unsigned char *data;
    SDL_Surface *rv;

    FILE *file = fopen(filename, "rb");
    if (!file)
        return 0;

    data = stbi_load_from_file(file, &x, &y, &comp, 0);
    fclose(file);

    if (comp == 4) {
        rv = SDL_CreateRGBSurface(0, x, y, 32, rmask, gmask, bmask, amask);
    } else if (comp == 3) {
        rv = SDL_CreateRGBSurface(0, x, y, 24, rmask, gmask, bmask, 0);
    } else {
        stbi_image_free(data);
        return 0;
    }
	
	if (rv) {
		memcpy(rv->pixels, data, comp * x * y);
	} else {
		fprintf(stderr, "SDL couldn't make surface while loading '%s'. Error: %s\n", filename, SDL_GetError());
	}
	
    stbi_image_free(data);

    return rv;
}

EXPORT void save_image(SDL_Surface *surface, const char *name)
{
	const int w = surface->w, h = surface->h;
	SDL_Rect r = {0,0,w,h};
	SDL_Rect r2 = r;
	SDL_Surface *rv = SDL_CreateRGBSurface(0, w, h, 24, rmask, gmask, bmask, 0);
	SDL_UpperBlit( surface, &r, rv, &r2 );
	stbi_write_png(name, rv->w, rv->h, 3, rv->pixels, 3*rv->w);
	SDL_FreeSurface(rv);
}


