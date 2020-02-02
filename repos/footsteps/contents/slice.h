// ControlBase "sprite-like" class headers

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2010 Andi McClure
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <math.h>

struct subtexture_slice;

// Class representing "something like a sprite".
// "init" sets bounds for the slice, and allocates storage.
// "consume" inits the slice and loads in the contents of a PNG.
// "construct" is a virtual method that does some kind of subclass-specific initialization-- say, creating a texture.
// "load" calls consume, then construct.
struct slice {
	int width;
	int height;
	uint32_t **pixel;
	bool constructed;
	slice() {width = 0; height = 0; pixel = 0; constructed = false;}
	virtual ~slice() {if (pixel) {for(int c = 0; c < width; c++) delete pixel[c]; delete[] pixel;}}
	virtual void init(int _width, int _height)
		{ height = _height; width = _width; pixel = new uint32_t*[width];
		  for(int c = 0; c < width; c++) pixel[c] = new uint32_t[height]; }

	// Convenience
	void consume(const char *name, bool fail_ok = false);
	virtual void construct() { constructed = true; }
	void load(const char *name) { // Absolute path
		consume(name);
		construct();
	}
	void iload(const char *name); // "Internal load"
	
	virtual slice *clone();
};

#define PNG_FLOORTYPE(x) (x == 0x000000FF)

struct block {
	int x, y, width, height; unsigned int color;
	static block b(int x, int y, int width, int height, unsigned int color) { block b = {x, y, width, height, color}; return b; } // I don't want to screw with STL's default constructors right now
};

struct block_slice : public slice {
	vector<block> blocks;
	
	block_slice() : slice() {}
	virtual void construct();
};

struct texture_slice : public slice {
	unsigned int texture; // GLUint in disguise
	
	// When the window is resized, 
	static hash_map<void *, texture_slice *> reconstruct_registry; // Use hash map as an ugly set type.
	
	texture_slice(unsigned int _texture = 0) : slice(), texture(_texture) {}
	virtual ~texture_slice();
	
    subtexture_slice *sub(); // Full size
    subtexture_slice *sub(float x1, float y1, float x2, float y2);
    subtexture_slice *asub(int x1, int y1, int x2, int y2);
    
	virtual void construct();
};

// Note: always unconstructed, doesn't own texture
struct subtexture_slice : public texture_slice {
    float coords[4];
    
    subtexture_slice(unsigned int _texture = 0, float x1 = 0, float y1 = 0, float x2 = 1, float y2 = 1)
        : texture_slice(_texture) { coords[0] = x1; coords[1] = y1; coords[2] = x2; coords[3] = y2; }
};

// TODO: Store original pixel sizes somewhere?
struct texture_atlas {
    texture_slice *parent;
    hash_map<string, subtexture_slice *> images;
    texture_atlas() : parent(NULL) {}
};

// Takes a simple filename, not a full pathname
// Note: Atlases are keyed on image name, not basename, at present.
// (In other words if the packing script splits an atlas in two,
// this mechanism won't handle that for you)
hash_map<string, texture_atlas *> atlas_load(string xmlPath);