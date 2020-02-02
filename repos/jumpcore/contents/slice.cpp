// ControlBase "sprite-like" class code

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

#include "kludge.h"
#include "slice.h"
#include "internalfile.h"
#include "tinyxml.h"
#include "glCommon.h"

#define OVERSIZE_CENTER 0

extern "C" {
unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
}

// TODO
#define FileBombBox(a)

void slice::iload(const char *name) {
    char filename2[FILENAMESIZE];
    internalPath(filename2, name);
    load(filename2);
}

void slice::consume(const char *filename) {
	int iwidth,iheight;
	unsigned char *image = stbi_load(filename, &iwidth, &iheight, NULL, 4);
	
    if ( !image ){
		REALERR("Couldn't decode file: %s\n", filename);
		FileBombBox(filename);
	}
    
	init(iwidth, iheight);
	memcpy(data, image, sizeof(uint32_t)*iwidth*iheight);
	
	free(image); // FIXME: Mixing malloc and free in a single program... :(
}

slice * slice::clone() {
	slice *s = new slice();
	s->init(width, height);
	memcpy(s->data, data, width*height);
	return s;
}

hash_map<void *, texture_slice *> texture_slice::reconstruct_registry; // Use hash map as an ugly set type.

texture_slice::texture_slice(unsigned int _texture)
	: slice(), texture(_texture), twidth(0), theight(0), owned(true) {
	iuncrop(0,0,0,0);
}

void texture_slice::uncrop(float x1, float y1, float x2, float y2) {
	coords[0] = x1; coords[1] = y1; coords[2] = x2; coords[3] = y2;
	icoords[0] = x1*twidth; icoords[1] = y1*theight; coords[2] = x2*twidth; icoords[3] = x2*theight;
}

void texture_slice::iuncrop(int x1, int y1, int x2, int y2) {
	icoords[0] = x1; icoords[1] = y1; icoords[2] = x2; icoords[3] = y2;
	coords[0] = float(x1)/twidth; coords[1] = float(y1)/theight; coords[2] = float(x2)/twidth; coords[3] = float(y2)/theight;
}

void texture_slice::crop(float x1, float y1, float x2, float y2) {
	coords[0] *= x1; coords[1] *= y1; coords[2] *= x2; coords[3] *= y2;
	icoords[0] = x1*twidth; icoords[1] = y1*theight; coords[2] = x2*twidth; icoords[3] = x2*theight;
}

void texture_slice::icrop(int x1, int y1, int x2, int y2) {
	iuncrop(coords[0]+x1, coords[1]+y1, coords[0]+x2, coords[1]+y2);
}

texture_slice *texture_slice::sub() {
	texture_slice *result = new texture_slice(texture);
	result->twidth = twidth; result->theight = theight;
	memcpy(result->coords, coords, sizeof(coords));
	memcpy(result->icoords, icoords, sizeof(icoords));
	result->owned = false;
	return result;
}

texture_slice *texture_slice::sub(float x1, float y1, float x2, float y2) {
	texture_slice *result = sub();
	result->crop(x1,y1,x2,y2);
	return result;
}
texture_slice *texture_slice::asub(int x1, int y1, int x2, int y2) {
	texture_slice *result = sub();
	result->icrop(x1,y1,x2,y2);
	return result;
}

texture_slice::~texture_slice() {
	if (constructed)
		reconstruct_registry.erase(this);
	vacate(); // Surely this is a noop if not constructed, tho?
}

void texture_slice::vacate() {
	if (texture)
		glDeleteTextures(1, (GLuint*)&texture);
	slice::vacate();
}

void texture_slice::construct() {	
	if (width > 0 && height > 0) { // If slice has been inited
		// First adjust size
		twidth = width; theight = height;
		iuncrop(0, 0, twidth, theight);
	
		if (!texture) // TODO: Something special for invalidated textures?
			glGenTextures(1, (GLuint *)&texture);
		
		// TODO: Test on PPC-endian system
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#if 0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, twidth, theight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, data);	

		if (!constructed) {
			reconstruct_registry[this] = this;
			constructed = true;
		}
	}
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
            
            atlas->images[subimagename] = atlas->parent->sub( left, top, right-left, bottom-top );
        }            
	}    
    
    return result;
}
