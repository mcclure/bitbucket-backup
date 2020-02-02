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
#include "tinyxml.h"

void FileBombBox(string filename); // This is supplied in display.cpp. If you don't want to use this, uncomment this line: 
// #define FileBombBox(a) Quit(1)

void slice::iload(const char *name) {
    char filename2[FILENAMESIZE];
    internalPath(filename2, name);
    load(filename2);
}

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

    if ( !image ){
		REALERR("Couldn't decode file: %s\n", filename);
		FileBombBox(filename);
	}
    
	init(decoder.infoPng.width, decoder.infoPng.height);
	uint32_t *image2 = (uint32_t *)image;
	for(int x = 0; x < decoder.infoPng.width; x++) {
		for(int y = 0; y < decoder.infoPng.height; y++) {
			pixel[x][y] = htonl(image2[y * decoder.infoPng.width + x]);
		}
	}	
	
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
			unsigned int color = ntohl( pixel[x][y] );
			const int off = (x+xo)+(y+yo)*twidth;
			textureData[off] = color;
		}
	}	
	
	glGenTextures(1, (GLuint *)&texture);
	
	// TODO: Test on PPC-endian system
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, twidth, theight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);	
	
	reconstruct_registry[this] = this;
	constructed = true;
}

subtexture_slice *texture_slice::sub() { // Full size
    return new subtexture_slice(texture);
}

subtexture_slice *texture_slice::sub(float x1, float y1, float x2, float y2) {
    subtexture_slice *s = new subtexture_slice(texture, x1, y1, x2, y2);
	s->rx = (x2-x1)*width + 0.5; s->ry = (y2-y1)*height + 0.5;
	return s;
}

subtexture_slice *texture_slice::asub(int x1, int y1, int x2, int y2) {
    return new subtexture_slice(texture, float(x1)/width, float(y1)/height, float(x2)/width, float(y2)/height);
}

hash_map<string, texture_atlas *> atlas_load(string xmlPath) {
    hash_map<string, texture_atlas *> result;
    
    char filename2[FILENAMESIZE];
    internalPath(filename2, xmlPath.c_str());
    TiXmlDocument xml(filename2);
    xml.LoadFile();
    
    if (!xml.RootElement()) {
        ERR("Couldn't load atlas xml file '%s' somehow?\n", filename2);
        FileBombBox(filename2); return result;
    }
    
    for( TiXmlNode *_atlasxml = xml.RootElement()->FirstChild(); _atlasxml; _atlasxml = _atlasxml->NextSibling() ) {
        if (_atlasxml->Type() != TiXmlNode::ELEMENT || _atlasxml->ValueStr() != "atlas") continue;
        TiXmlElement *atlasxml = (TiXmlElement *)_atlasxml;
        
        const char *imagename = atlasxml->Attribute("filename");
        texture_atlas *atlas = new texture_atlas();
        
        result[imagename] = atlas;
        internalPath(filename2, imagename);
        atlas->parent = new texture_slice();
        atlas->parent->load(filename2);
        
        for( TiXmlNode *_imagexml = atlasxml->FirstChild(); _imagexml; _imagexml = _imagexml->NextSibling() ) {
            if (_imagexml->Type() != TiXmlNode::ELEMENT || _imagexml->ValueStr() != "image") continue;
            TiXmlElement *imagexml = (TiXmlElement *)_imagexml;
            
            const char *subimagename = imagexml->Attribute("name");
            float bottom, top, left, right;
            if (!subimagename
                || TIXML_SUCCESS != imagexml->QueryFloatAttribute("bottom", &bottom)
                || TIXML_SUCCESS != imagexml->QueryFloatAttribute("top", &top)
                || TIXML_SUCCESS != imagexml->QueryFloatAttribute("left", &left)
                || TIXML_SUCCESS != imagexml->QueryFloatAttribute("right", &right)) continue;
            
            atlas->images[subimagename] = atlas->parent->sub( left, top, right, bottom );
        }            
	}    
    
    return result;
}