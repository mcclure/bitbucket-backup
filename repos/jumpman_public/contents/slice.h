/*
 *  controls.h
 *  Jumpman
 *
 *  Created by Andi McClure on 4/3/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <math.h>

using namespace std;

struct block {
	int x, y, width, height; unsigned int color;
	static block b(int x, int y, int width, int height, unsigned int color) { block b = {x, y, width, height, color}; return b; } // I don't want to screw with STL's default constructors right now
};

class LodePNG_Decoder;
class spaceinfo;

extern bool special_ending_floor_construction;

struct slice {
       int width;
       int height;
       uint32_t **pixel;
	   vector<block> blocks;
       slice() {width = 0; height = 0; pixel = 0;}
       ~slice() {if (pixel) {for(int c = 0; c < width; c++) delete pixel[c]; delete[] pixel;}}
       void init(int width, int height) { this->height = height; this->width = 
width; pixel = new uint32_t*[width]; for(int c = 0; c < width; c++) pixel[c] = new
uint32_t[height]; }
	   void consume(LodePNG_Decoder &decoder, unsigned char* image);
	   void construct();
	   
	   // Convenience
	   void construct(LodePNG_Decoder &decoder, unsigned char* image, spaceinfo *tiltfrom = NULL);
	   void construct(const char *name, bool reallyconstruct = true, spaceinfo *tiltfrom = NULL);
	   
	   void border(spaceinfo *tiltfrom); // Call between consume and construct to (maybe) add a repeat/wrap border.
	   slice *clone(); // Pixels only, no blocks
};