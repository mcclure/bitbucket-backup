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

#ifndef _SLICE_H
#define _SLICE_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <math.h>

struct texture_slice;

// Class representing "something like a sprite".
// "init" sets bounds for the slice, and allocates storage.
// "consume" inits the slice and loads in the contents of a PNG.
// "construct" is a virtual method that does some kind of subclass-specific initialization-- say, creating a texture.
// "load" calls consume, then construct.
struct slice {
	int width; // Refers to width, height of "backing store".
	int height;
	
	uint32_t *data;
	uint32_t &pixel(int x, int y) { return data[y*width+x]; }
	bool contains(int x, int y) { return x >= 0 && y >= 0 && x < width && y < height; }
	
	bool constructed;
	slice() : width(0), height(0), data(NULL), constructed(false) {}
	virtual ~slice() {deallocate();}
	void deallocate() {	delete[] data; data=NULL; } // Inverse of init
	virtual void init(int _width, int _height, bool _has_backing = true) { 
		vacate(); deallocate();
		height = _height; width = _width; 
		if (_has_backing) data = new uint32_t[width*height];
	}

	// Convenience
	void consume(const char *name);
	virtual void construct() { constructed = true; }
	virtual void vacate() { constructed = false; } // Inverse of construct
	void load(const char *name) { // Absolute path
		consume(name);
		construct();
	}
	void iload(const char *name); // "Internal load"
	
	bool has_backing() { return data; } // has_backing is a property defined by pixel's NULLNESS
	
	virtual slice *clone();
};

// FIXME: Really unhappy with the sheer number of fields here.
struct texture_slice : public slice {
	unsigned int texture; // GLUint in disguise
	int twidth, theight;
	float coords[4];
	float icoords[4]; // May be more accurate than coords can offer.
	bool owned;
	
	// When the window is resized, 
	static hash_map<void *, texture_slice *> reconstruct_registry; // Use hash map as an ugly set type.
	
	texture_slice(unsigned int _texture = 0);
	virtual ~texture_slice();
	
	void uncrop(float x1 = 0, float y1 = 0, float x2 = 1, float y2 = 1); // "Crop from absolute 0..1"
	void crop(float x1, float y1, float x2, float y2); // "Crop relative to current"
	void iuncrop(int x1, int y1, int x2, int y2);
	void icrop(int x1, int y1, int x2, int y2);
    texture_slice *sub(); // Full size, unowned.
    texture_slice *sub(float x1, float y1, float x2, float y2);
    texture_slice *asub(int x1, int y1, int x2, int y2);
    
	int sub_width() { return icoords[2]-icoords[0]; }
	int sub_height() { return icoords[3]-icoords[1]; }
	
	void construct();
	void vacate();
	
	virtual void texture_swap(texture_slice *other) { // Swap texture *only*
		unsigned int temp = other->texture;
		other->texture = texture;
		texture = temp;
	}
};

// TODO: Store original pixel sizes somewhere?
struct texture_atlas {
    texture_slice *parent;
    hash_map<string, texture_slice *> images;
    texture_atlas() : parent(NULL) {}
};

// Takes a simple filename, not a full pathname
// Note: Atlases are keyed on image name, not basename, at present.
// (In other words if the packing script splits an atlas in two,
// this mechanism won't handle that for you)
hash_map<string, texture_atlas *> atlas_load(string xmlPath);

#endif