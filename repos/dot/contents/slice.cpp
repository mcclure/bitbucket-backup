// ControlBase "sprite-like" class code

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

#include "kludge.h"
#include "slice.h"
#include "lodepng.h"
#include "internalfile.h"

void FileBombBox(string filename); // This is supplied in display.cpp. If you don't want to use this, uncomment this line: 
// #define FileBombBox(a) Quit(1)

void slice::consume(const char *filename) {
	LodePNG_Decoder decoder;
	LodePNG_Decoder_init(&decoder);
	unsigned char* buffer;
	unsigned char* image;
	size_t buffersize, imagesize;
	
	LodePNG_loadFile(&buffer, &buffersize, filename); /*load the image file with given filename*/
	
	if ( !buffer || buffersize <= 0 ){
		REALERR("Couldn't open file: %s\n", filename);
		FileBombBox(filename);
	}
	
	LodePNG_decode(&decoder, &image, &imagesize, buffer, buffersize); /*decode the png*/
	
	int width = decoder.infoPng.width; height = decoder.infoPng.height;
	
	init(width, height);
	uint32_t *image2 = (uint32_t *)image;
	for(int x = 0; x < decoder.infoPng.width; x++) {
		for(int y = 0; y < decoder.infoPng.height; y++) {
			pixel[x][y] = htonl(image2[y * decoder.infoPng.width + x]);
		}
	}	
	
#if NAMETRACK
	name = filenamestrip(filename);
#endif
	
	free(buffer); free(image); // FIXME: Mixing malloc and free in a single program... :(
}

slice * slice::clone() {
	slice *s = new slice();
	s->init(width, height);
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			s->pixel[x][y] = pixel[x][y];
		}
	}
	return s;
}

hash_map<void *, texture_slice *> texture_slice::reconstruct_registry; // Use hash map as an ugly set type.

texture_slice::~texture_slice() {
	if (constructed) {
		reconstruct_registry.erase(this);
		glDeleteTextures(1, (GLuint*)&texture);
	}
}

void texture_slice::construct() {
	int twidth = width, theight = height;
	int xo = 0, yo = 0;
	if (twidth > theight) { yo = (twidth-theight)/2; theight = twidth; }
	if (theight > twidth) { xo = (theight-twidth)/2; twidth = theight; }	
	GLuint textureData[twidth*theight];
	memset(textureData, 0, sizeof(textureData));
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			unsigned int color = ntohl( pixel[width-x-1][height-y-1] );
			const int off = (x+xo)+(y+yo)*twidth;
			textureData[off] = color;
		}
	}	
	
	glGenTextures(1, (GLuint *)&texture);
	
	// TODO: Test on PPC-endian system
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, twidth, theight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);	
	
	reconstruct_registry[this] = this;
	constructed = true;
}

string filenamestrip(string filename) {
	int idx = filename.rfind('/');
	if (idx >= 0 && idx < filename.size()-1)
		filename = filename.substr(idx+1);
	idx = filename.rfind('.');
	if (idx > 0)
		filename = filename.substr(0,idx);
	return filename;
}