// ControlBase "sprite-like" class headers

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andi McClure
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

using namespace std;

// Real quick, some magical incantations to allow the use of hash_map later:
#include <ext/hash_map>
using namespace ::__gnu_cxx;
namespace __gnu_cxx {                                                                                             
	template<> struct hash< std::string > // Allow STL strings with hash_map
	{ size_t operator()( const std::string& x ) const { return hash< const char* >()( x.c_str() ); } };          
	template<> struct hash< void * > // Allow pointers with hash_map               
	{ size_t operator()( const void * x ) const { return hash< unsigned int >()( (unsigned int)x ); } };          
}          

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
#if NAMETRACK
	string name;
#endif
	slice() {width = 0; height = 0; pixel = 0; constructed = false;}
	virtual ~slice() {if (pixel) {for(int c = 0; c < width; c++) delete pixel[c]; delete[] pixel;}}
	virtual void init(int width, int height)
		{ this->height = height; this->width = width; pixel = new uint32_t*[width];
		  for(int c = 0; c < width; c++) pixel[c] = new uint32_t[height]; }

	// Convenience
	void consume(const char *name);
	virtual void construct() { constructed = true; }
	void load(const char *name) {
		consume(name);
		construct();
	}
	
	virtual slice *clone();
};

struct texture_slice : public slice {
	unsigned int texture; // GLUint in disguise
	
	// When the window is resized, 
	static hash_map<void *, texture_slice *> reconstruct_registry; // Use hash map as an ugly set type.
	
	texture_slice() : slice() {}
	virtual ~texture_slice();
	virtual slice *clone() { slice *c = slice::clone(); if (constructed) c->construct(); return c; }
	
	virtual void construct();
};

string filenamestrip(string filename);